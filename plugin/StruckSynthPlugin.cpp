// StruckSynthPlugin.cpp — DSP side: DPF Plugin wrapper around the vendored PolygraphModular
// voice engine. DPF hands us MIDI 1.0 events + a parameter interface and generates the CLAP
// (VST3/LV2) wrapper; all the synthesis is the same VoiceManager the self-tests exercise.
//
//   MIDI events  → pm::UmpMessage → VoiceManager (polyphony / stealing / mono)
//   params       → VoiceManager::setParameter() (propagates to every voice, live)
//   run()        → chunked VoiceManager::process()
//
// The v1 voice is Strike (exciter) → ModalBank (resonator) → Final; see buildDefaultPatch.

#include "DistrhoPlugin.hpp"

#include <algorithm>
#include <memory>

#include "StruckSynthParams.h"

#include "ParamAddressMacros.h"
#include "VoiceManager.hpp"
#include "pm_addresses.h"
#include "pm_midi.h"
#include "pm_params.h"

START_NAMESPACE_DISTRHO

using namespace ifsynth;

class IFSynthPlugin : public Plugin
{
public:
    IFSynthPlugin()
        : Plugin(kParamCount, 0, 0) // params, programs, states
    {
        for (uint32_t i = 0; i < kParamCount; ++i)
            fValues[i] = kParams[i].def;
        initVoiceManager();
    }

protected:
    /* -- information ------------------------------------------------------ */
    const char* getLabel()       const override { return "StruckSynth"; }
    const char* getDescription() const override
    {
        return "A modal-synthesis (struck-resonator) instrument built on the PolygraphModular voice engine.";
    }
    const char* getMaker()    const override { return "Unlikelyware"; }
    const char* getHomePage() const override { return "https://unlikelyware.com/strucksynth"; }
    const char* getLicense()  const override { return "MIT"; }
    uint32_t    getVersion()  const override { return d_version(0, 4, 0); }
    int64_t     getUniqueId() const override { return d_cconst('S', 't', 'r', 'k'); }

    /* -- init ------------------------------------------------------------- */
    void initAudioPort(bool input, uint32_t index, AudioPort& port) override
    {
        port.groupId = kPortGroupStereo; // 0 inputs; two output channels form a stereo pair
        Plugin::initAudioPort(input, index, port);
    }

    void initParameter(uint32_t index, Parameter& parameter) override
    {
        if (index >= kParamCount)
            return;
        const ParamSpec& s = kParams[index];
        parameter.hints      = kParameterIsAutomatable
                             | (s.logarithmic ? kParameterIsLogarithmic : 0u)
                             | (s.integer     ? kParameterIsInteger     : 0u);
        parameter.name       = s.name;
        parameter.symbol     = s.symbol;
        parameter.unit       = s.unit;
        parameter.ranges.min = s.min;
        parameter.ranges.max = s.max;
        parameter.ranges.def = s.def;
    }

    float getParameterValue(uint32_t index) const override
    {
        return index < kParamCount ? fValues[index] : 0.0f;
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (index >= kParamCount)
            return;
        fValues[index] = value;
        applyParam(index, value);
    }

    /* -- lifecycle -------------------------------------------------------- */
    void sampleRateChanged(double) override { initVoiceManager(); }
    void activate()               override { if (fVoiceManager) fVoiceManager->reset(); }

    /* -- process ---------------------------------------------------------- */
    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        float* outL = outputs[0];
        float* outR = outputs[1];

        uint32_t pos = 0, ev = 0;
        while (pos < frames)
        {
            // Dispatch every MIDI event scheduled at or before the current frame.
            for (; ev < midiEventCount && midiEvents[ev].frame <= pos; ++ev)
                dispatchMidi(midiEvents[ev]);

            uint32_t next = frames;
            if (ev < midiEventCount && midiEvents[ev].frame < frames)
                next = midiEvents[ev].frame;

            renderSegment(outL, outR, pos, next);
            pos = next;
        }
    }

private:
    std::unique_ptr<VoiceManager> fVoiceManager;
    float                         fValues[kParamCount];

    // Map a DPF param index onto its VoiceManager module item address. Globals handled specially
    // in applyParam (voice mode + glide) have no direct address here.
    static AUParameterAddress vmAddress(uint32_t paramIndex)
    {
        switch (paramIndex)
        {
        // Master output / stereo (ModalBank's own output stage)
        case kParamLevel:          return MODULE_PARAM_ADDRESS(ModuleParamKindModalBank, ModuleInstance1, ModalBankParamItemLevel);
        case kParamSpread:         return MODULE_PARAM_ADDRESS(ModuleParamKindModalBank, ModuleInstance1, ModalBankParamItemSpread);
        // Strike (exciter)
        case kParamStrikeType:     return MODULE_PARAM_ADDRESS(ModuleParamKindStrike, ModuleInstance1, StrikeParamItemType);
        case kParamHardness:       return MODULE_PARAM_ADDRESS(ModuleParamKindStrike, ModuleInstance1, StrikeParamItemHardness);
        case kParamVelToLevel:     return MODULE_PARAM_ADDRESS(ModuleParamKindStrike, ModuleInstance1, StrikeParamItemVelToLevel);
        case kParamVelToHardness:  return MODULE_PARAM_ADDRESS(ModuleParamKindStrike, ModuleInstance1, StrikeParamItemVelToHardness);
        case kParamContactMode:    return MODULE_PARAM_ADDRESS(ModuleParamKindStrike, ModuleInstance1, StrikeParamItemContactMode);
        // ModalBank: object
        case kParamPitch:          return MODULE_PARAM_ADDRESS(ModuleParamKindModalBank, ModuleInstance1, ModalBankParamItemPitch);
        case kParamMaterial:       return MODULE_PARAM_ADDRESS(ModuleParamKindModalBank, ModuleInstance1, ModalBankParamItemMaterial);
        case kParamModeCount:      return MODULE_PARAM_ADDRESS(ModuleParamKindModalBank, ModuleInstance1, ModalBankParamItemModeCount);
        case kParamStretch:        return MODULE_PARAM_ADDRESS(ModuleParamKindModalBank, ModuleInstance1, ModalBankParamItemStretch);
        // ModalBank: decay
        case kParamDecayT60:       return MODULE_PARAM_ADDRESS(ModuleParamKindModalBank, ModuleInstance1, ModalBankParamItemDecayT60);
        case kParamDampTilt:       return MODULE_PARAM_ADDRESS(ModuleParamKindModalBank, ModuleInstance1, ModalBankParamItemDampTilt);
        case kParamDampBellAmount: return MODULE_PARAM_ADDRESS(ModuleParamKindModalBank, ModuleInstance1, ModalBankParamItemDampBellAmount);
        case kParamDampBellCentre: return MODULE_PARAM_ADDRESS(ModuleParamKindModalBank, ModuleInstance1, ModalBankParamItemDampBellCentre);
        case kParamDampBellWidth:  return MODULE_PARAM_ADDRESS(ModuleParamKindModalBank, ModuleInstance1, ModalBankParamItemDampBellWidth);
        // ModalBank: timbre
        case kParamStrikePosition: return MODULE_PARAM_ADDRESS(ModuleParamKindModalBank, ModuleInstance1, ModalBankParamItemStrikePosition);
        case kParamBrightness:     return MODULE_PARAM_ADDRESS(ModuleParamKindModalBank, ModuleInstance1, ModalBankParamItemBrightness);
        default:                   return 0;
        }
    }

    static bool isMono(float voiceModeValue)
    {
        return static_cast<int>(voiceModeValue + 0.5f) == 0; // 0 = Mono, 1 = poly
    }

    // Voice mode: Mono = 1 voice + VoiceManager legato (last-note note-stack); Poly = 10 voices.
    // The modal voice has no amplitude envelope, so there is no envelope legato to configure.
    void applyVoiceMode(float value)
    {
        if (!fVoiceManager)
            return;
        const bool mono = isMono(value);
        setConfig(MIDIParamItemVoiceCount, mono ? 1.0f : 10.0f);
        setConfig(MIDIParamItemLegato,     mono ? 1.0f : 0.0f);
        applySlew();
    }

    // Pitch glide (Slew Rise == Fall). Only active in mono; in poly it is forced near-instant so
    // each freshly allocated voice doesn't audibly scoop from its rest pitch.
    void applySlew()
    {
        if (!fVoiceManager)
            return;
        const float rate = isMono(fValues[kParamVoiceMode]) ? fValues[kParamGlide] : 1e-8f;
        fVoiceManager->setParameter(slew1RiseAddress, rate);
        fVoiceManager->setParameter(slew1FallAddress, rate);
    }

    // Route one param to the engine: globals are handled here; everything else is a module param.
    void applyParam(uint32_t index, float value)
    {
        if (!fVoiceManager)
            return;
        if (index == kParamGlide)     { applySlew();          return; }
        if (index == kParamVoiceMode) { applyVoiceMode(value); return; }
        const AUParameterAddress a = vmAddress(index);
        if (a != 0)
            fVoiceManager->setParameter(a, value);
    }

    // (Re)build the VoiceManager for the current sample rate and push all defaults into it.
    void initVoiceManager()
    {
        fVoiceManager = std::make_unique<VoiceManager>(getSampleRate(), MODULE_BUFF_SIZE);

        // VoiceManager params aren't zero-initialised. Voice count + legato are driven by the
        // Voices param (applyVoiceMode, pushed in the loop below); set the rest here.
        setConfig(MIDIParamItemChannelFilterValue, 0);
        setConfig(MIDIParamItemGroupFilterValue, 0);
        setConfig(MIDIParamItemVoiceStealAlgorithm, VoiceStealAlgorithmOldest);
        setConfig(MIDIParamItemFreeRun, 0);

        // Push every param so the engine matches the exposed parameter values.
        for (uint32_t i = 0; i < kParamCount; ++i)
            applyParam(i, fValues[i]);
    }

    void setConfig(MIDIParamItem item, float v)
    {
        fVoiceManager->setVoiceManagerParameter(
            MODULE_PARAM_ADDRESS(ModuleParamKindMIDI, ModuleInstance1, item), v);
    }

    void dispatchMidi(const MidiEvent& me)
    {
        const uint8_t* d = me.size > MidiEvent::kDataSize ? me.dataExt : me.data;
        const pm::UmpMessage m = pm::fromMidi1Bytes(d[0],
                                                    me.size > 1 ? d[1] : 0,
                                                    me.size > 2 ? d[2] : 0);
        if (pm::isChannelMessage(m))
            fVoiceManager->handleMIDIMessage(m);
    }

    // Render [start,end) by chunking to the module block size; VoiceManager zeroes + sums voices.
    // Stereo width comes from the ModalBank's own Spread param, so there is no output-bus M/S here.
    void renderSegment(float* outL, float* outR, uint32_t start, uint32_t end)
    {
        for (uint32_t p = start; p < end;)
        {
            const uint32_t chunk = std::min<uint32_t>(end - p, MODULE_BUFF_SIZE);
            fVoiceManager->process(chunk, outL + p, outR + p);
            p += chunk;
        }
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IFSynthPlugin)
};

Plugin* createPlugin()
{
    return new IFSynthPlugin();
}

END_NAMESPACE_DISTRHO
