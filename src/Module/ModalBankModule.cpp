//
//  ModalBankModule.mm
//

#include "ModalBankModule.hpp"
#include <cmath>
#include "ModalBank.hpp"          // the framework-free core
#include "ModalCloud.hpp"         // the Gaussian mode cloud
#include "PitchConverter.hpp"
#include "pm_midi.h"

#include <algorithm>
#include <atomic>

// StruckGaussian's object is always a mode cloud (ModalCloud.hpp). Its modes are
// generated into the state's buffer and that buffer handed to the core; the
// pointer never changes, so a regenerated cloud needs bank.invalidate().
//
// Trim. The bank normalises energy, not peak, and every mode starts in phase at
// a strike, so a cloud of near-equal modes hits 6-8 dB harder than a table such
// as Bar, and more modes add more click (tools/cloudlab). The strike is scaled
// by kTrimRef / (the bank's in-phase peak), so Modes, Width and Centre leave the
// level of the hit where Bar's is (Focus included: a narrow loudness peak would
// otherwise hit softer). It scales the INPUT, so a trim that moves
// (Strike Position, a glide) changes the next strike, never a ringing note.
struct ModalBankState {
    ModalBank     bank;
    ModalMode     cloud[ModalBank::kMaxModes];
    ModalMaterial cloudMaterial{ cloud, 0, "Cloud" };

    // what the cloud in the buffer was generated from; count 0 = nothing yet
    int      count = 0;
    float    centre = 0.0f, width = 0.0f;
    float    focus = 0.0f, focusCentre = 0.0f, focusWidth = 0.5f;
    uint32_t seed = 0;
    bool     keepFund = true;

    bool     noteOn = false;     // a note-on since the last block: Reseed draws here
    uint32_t noteSeed = 1;       // the seed this note plays, with Reseed on

    float    trim = 1.0f;
    float    trimPos = -1.0f, trimBright = 0.0f, trimStretch = 0.0f, trimHz = 0.0f;
    bool     trimDirty = true;

    // Ring mod: a free-running sine (its phase never resets on a strike, which would click a
    // ringing note) and the level the last block ended at, so a moved knob ramps.
    double   sampleRate = 48000.0;
    double   ringPhase = 0.0;      // cycles, 0..1
    float    ringLevel = 0.0f;
};

// The in-phase peak of the Bar table at the plugin's defaults (C4, Strike
// Position 0.25, Brightness 0, Stretch 0), from ModalBank::modeAmplitude: the
// level a trimmed cloud hits at.
static constexpr float kTrimRef = 2.1181f;

// A new seed for every Reseed note, shared by all voices so two voices struck
// together draw different clouds.
static uint32_t nextNoteSeed(uint32_t base)
{
    static std::atomic<uint32_t> counter{0};
    uint32_t x = base * 0x9E3779B1u + (++counter) * 0x85EBCA77u;
    x ^= x >> 16; x *= 0x7FEB352Du; x ^= x >> 15;       // mix, so neighbours differ
    return x;
}

// Regenerate the cloud when anything it is drawn from has changed. Returns true
// if it did.
static bool updateCloud(Module* module, ModalBankState* st)
{
    const int   count    = std::clamp(module->getParamValueAsInt(ModalBankParamItemModeCount), 1, ModalBank::kMaxModes);
    const float centre   = module->getParamValue(ModalBankParamItemCloudCentre);
    const float width    = std::max(module->getParamValue(ModalBankParamItemCloudWidth), 0.0f);
    const bool  keepFund = module->getParamValue(ModalBankParamItemCloudKeepFund) >= 0.5f;
    const bool  reseed   = module->getParamValue(ModalBankParamItemCloudReseed) >= 0.5f;
    const uint32_t base  = (uint32_t)std::max(1, module->getParamValueAsInt(ModalBankParamItemCloudSeed));
    const float focus    = std::clamp(module->getParamValue(ModalBankParamItemCloudFocus), 0.0f, 1.0f);
    const float fCentre  = module->getParamValue(ModalBankParamItemCloudFocusCentre);
    const float fWidth   = module->getParamValue(ModalBankParamItemCloudFocusWidth);

    // Reseed off: the seed itself, so a preset's cloud is the one cloudlab draws.
    if (reseed && st->noteOn) st->noteSeed = nextNoteSeed(base);
    st->noteOn = false;
    const uint32_t seed = reseed ? st->noteSeed : base;

    if (count == st->count && centre == st->centre && width == st->width
        && seed == st->seed && keepFund == st->keepFund
        && focus == st->focus && fCentre == st->focusCentre && fWidth == st->focusWidth)
        return false;

    st->count = count; st->centre = centre; st->width = width; st->seed = seed; st->keepFund = keepFund;
    st->focus = focus; st->focusCentre = fCentre; st->focusWidth = fWidth;
    st->cloudMaterial.count = ModalCloud::generate(count, centre, width, seed, keepFund,
                                                   st->cloud, ModalBank::kMaxModes,
                                                   focus, fCentre, fWidth);
    st->bank.setMaterial(st->cloudMaterial);
    st->bank.setModeCount(st->cloudMaterial.count);
    st->bank.invalidate();
    return true;
}

const void InitModalBankModule(uint32_t /*count*/, Module* module)
{
    auto* st = new ModalBankState();
    st->bank.setSampleRate(module->mSampleRate);
    st->sampleRate = module->mSampleRate;
    st->bank.setPositionWeighting(false);   // a cloud has no string's mode shapes (ModalBank.hpp)
    module->state = st;
}

const void DeinitModalBankModule(Module* module)
{
    delete static_cast<ModalBankState*>(module->state);
    module->state = nullptr;
}

void MIDIModalBankModule(const pm::UmpMessage& message, Module* module)
{
    if (auto* st = static_cast<ModalBankState*>(module->state)) {
        if (pm::isNoteOn(message)) st->noteOn = true;
    }
}

const void ResetModalBankModule(Module* module)
{
    // Voice allocation only — NOT note-on. Re-striking a ringing bank means
    // adding excitation to a system already in motion; resetting here would
    // click and destroy the ring-down. See ModalSynthesis.md.
    if (auto* st = static_cast<ModalBankState*>(module->state)) {
        st->bank.reset();
        st->ringPhase = 0.0;
    }
}

const void SampleRateModalBankModule(Module* module, double newSampleRate)
{
    if (auto* st = static_cast<ModalBankState*>(module->state)) {
        st->bank.setSampleRate(newSampleRate);
        st->sampleRate = newSampleRate;
    }
}

// Read a control-style input as a single per-block value. The mod ports here
// (pitch, decay, brightness, strike) all carry slow control signals, and the
// core recomputes coefficients per block regardless, so a per-sample read would
// buy nothing. Returns 0 when nothing is patched.
//
// The LAST sample, not the first. A new note's pitch reaches the bank through
// the Slew, which moves at most 0.21 octave a sample even at its fastest (its
// 0.1 ms-per-octave floor), so sample 0 still holds about the voice's previous
// pitch. Read there, the whole first block (the strike included) rang at the
// old note's pitch and retuned a block later: a pitch blip and a level change
// on every strike that changed note, and on a voice's first note (from 0
// octaves, A440). The last sample is where the Slew has arrived.
static inline float readControl(Module* module, uint8_t key,
                                uint64_t processCallID, uint32_t count)
{
    if (!module->isInputConnected(key) || count == 0) return 0.0f;
    const KPBuffPair* p = module->retrieveInput(key, processCallID, count);
    return p->first->data[count - 1];
}

const KPBuffPair* ProcessModalBankModule(uint64_t processCallID, uint32_t count, Module* module)
{
    auto* st = static_cast<ModalBankState*>(module->state);
    if (!st) return KPBuffPairHelper::zeroes();

    ModalBank& bank = st->bank;

    // --- pitch: PM octaves -> Hz -------------------------------------------
    // octaves = Pitch (transpose) + PitchModLevel * PitchInput (note tracking)
    //         + PitchMod2Level * PitchMod2Input (pitch bend).
    // PitchModLevel defaults to 1 (unity tracking) and 0 gives a fixed-pitch
    // body resonator. The bend input is scaled separately, so it still bends one.
    const float pitchParam = module->getParamValue(ModalBankParamItemPitch);
    const float pitchLevel = module->getParamValue(ModalBankParamItemPitchModLevel);
    const float pitchIn    = readControl(module, ModalBankParamItemPitchInput, processCallID, count);
    const float bendLevel  = module->getParamValue(ModalBankParamItemPitchMod2Level);
    const float bendIn     = readControl(module, ModalBankParamItemPitchMod2Input, processCallID, count);
    const float octaves    = pitchParam + pitchLevel * pitchIn + bendLevel * bendIn;
    bank.setFundamental((float)module->pitchConverter->pitchToFrequency(octaves));

    // --- object ------------------------------------------------------------
    if (updateCloud(module, st)) st->trimDirty = true;
    // Stretch is an INDEX into the shared value table, not a raw float.
    bank.setStretch(kModalStretchValues[
        std::clamp(module->getParamValueAsInt(ModalBankParamItemStretch),
                   0, kModalStretchCount - 1)]);

    // --- decay, with optional modulation -----------------------------------
    // Decay modulation is in octaves of T60 (+1 doubles it, -1 halves it), so one amount feels the
    // same on a short wood block and a long bell. Both these inputs act on a ringing note: they
    // change each mode's decay coefficient, not just the next strike.
    const float decayMod = readControl(module, ModalBankParamItemDecayModInput, processCallID, count)
                         * module->getParamValue(ModalBankParamItemDecayModLevel);
    bank.setDecayT60(module->getParamValue(ModalBankParamItemDecayT60) * std::exp2(decayMod));
    const float tiltMod = readControl(module, ModalBankParamItemDampTiltModInput, processCallID, count)
                        * module->getParamValue(ModalBankParamItemDampTiltModLevel);
    bank.setDampTilt(module->getParamValue(ModalBankParamItemDampTilt) + tiltMod);
    bank.setDampBellAmount(module->getParamValue(ModalBankParamItemDampBellAmount));
    bank.setDampBellCentre(module->getParamValue(ModalBankParamItemDampBellCentre));
    bank.setDampBellWidth(module->getParamValue(ModalBankParamItemDampBellWidth));

    // --- strike, with optional modulation ----------------------------------
    // Strike position only shapes how a strike excites the modes (it is an input gain), so moving it
    // matters at the strike, not on a note already ringing.
    const float strikeMod = readControl(module, ModalBankParamItemStrikeInput, processCallID, count)
                          * module->getParamValue(ModalBankParamItemStrikeModLevel);
    bank.setStrikePosition(module->getParamValue(ModalBankParamItemStrikePosition) + strikeMod);

    const float brightMod = readControl(module, ModalBankParamItemBrightModInput, processCallID, count)
                          * module->getParamValue(ModalBankParamItemBrightModLevel);
    bank.setBrightness(module->getParamValue(ModalBankParamItemBrightness) + brightMod);

    // --- output ------------------------------------------------------------
    bank.setSpread(module->getParamValue(ModalBankParamItemSpread));

    // Excitation. If nothing is patched we still process with a null exciter so
    // a previously struck bank keeps ringing down — the common case. The core
    // treats null as "no input this block", not silence.
    const float* exc = nullptr;
    if (module->isInputConnected(ModalBankParamItemInput)) {
        const KPBuffPair* in = module->retrieveInput(ModalBankParamItemInput, processCallID, count);
        exc = in->first->data;   // left channel drives the (mono-in) bank
    }

    float* outL = module->output.first->data;
    float* outR = module->output.second->data;

    // Trim, recomputed only when something that moves the in-phase peak has changed.
    {
        const float pos = module->getParamValue(ModalBankParamItemStrikePosition) + strikeMod;
        const float bright = module->getParamValue(ModalBankParamItemBrightness) + brightMod;
        const float stretch = kModalStretchValues[
            std::clamp(module->getParamValueAsInt(ModalBankParamItemStretch), 0, kModalStretchCount - 1)];
        const float hz = bank.modeFrequency(0);
        if (st->trimDirty || pos != st->trimPos || bright != st->trimBright
            || stretch != st->trimStretch || hz != st->trimHz) {
            bank.prepare();
            float peak = 0.0f;
            for (int i = 0; i < bank.activeModes(); ++i) peak += bank.modeAmplitude(i);
            st->trim = (peak > 1e-6f) ? kTrimRef / peak : 1.0f;
            st->trimPos = pos; st->trimBright = bright; st->trimStretch = stretch; st->trimHz = hz;
            st->trimDirty = false;
        }
    }

    if (exc == nullptr) {
        bank.processBlock(nullptr, outL, outR, (int)count);
    } else {
        // The trimmed strike, in chunks so no buffer has to be sized to the host's block.
        float trimmed[256];
        for (uint32_t done = 0; done < count; ) {
            const uint32_t n = std::min<uint32_t>(256, count - done);
            for (uint32_t i = 0; i < n; ++i) trimmed[i] = exc[done + i] * st->trim;
            bank.processBlock(trimmed, outL + done, outR + done, (int)n);
            done += n;
        }
    }

    const float level = module->getParamValue(ModalBankParamItemLevel);
    if (level != 1.0f) {
        for (uint32_t i = 0; i < count; ++i) { outL[i] *= level; outR[i] *= level; }
    }

    // --- ring mod ----------------------------------------------------------
    // out += L * out * sin: the ring-modulated signal is ADDED to the struck sound, which stays at
    // full level, so Ring 0 leaves the bank untouched. The sine tracks the note (with bend) but not the Pitch transpose, plus its own
    // offset. The level ramps across the block; the frequency is per block, phase-continuous.
    const float ringTarget = std::clamp(module->getParamValue(ModalBankParamItemRingLevel), 0.0f, 1.0f);
    if (ringTarget > 0.0f || st->ringLevel > 0.0f) {
        const float  ringOct = pitchLevel * pitchIn + bendLevel * bendIn
                             + module->getParamValue(ModalBankParamItemRingPitch);
        const double inc     = module->pitchConverter->pitchToFrequency(ringOct) / st->sampleRate;
        const float  step    = count > 0 ? (ringTarget - st->ringLevel) / (float)count : 0.0f;
        float  l     = st->ringLevel;
        double phase = st->ringPhase;
        for (uint32_t i = 0; i < count; ++i) {
            l += step;
            const float m = 1.0f + l * (float)std::sin(2.0 * M_PI * phase);
            outL[i] *= m; outR[i] *= m;
            phase += inc;
            if (phase >= 1.0) phase -= std::floor(phase);
        }
        st->ringPhase = phase;
    }
    st->ringLevel = ringTarget;

    return &module->output;
}
