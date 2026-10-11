// ss_bend_test — MIDI 2.0 per-note decode, channel + per-note pitch bend, and channel-aware voice
// matching, end to end through the VoiceManager (no plugin, no host).
//
// To measure pitch, the bank is set to a single mode of the Harmonic series: each voice is then one
// decaying sine at its fundamental, so a zero-crossing count (one note) or a single-bin DFT (several
// notes) reads the pitch directly. The T60 is long, so the tone barely decays over a measurement.
//
// A struck voice keeps ringing after note-off (the bank's decay is the envelope), so "this note was
// released" is checked through activeCount() after a short ring-out, not by listening for silence.

#include "VoiceManager.hpp"
#include "pm_addresses.h"
#include "pm_midi.h"
#include "pm_params.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

constexpr double   kSampleRate = 48000.0;
constexpr uint32_t kBlock      = 512;

int g_fails = 0;
void check(bool ok, const std::string& what) {
   std::printf("  %s: %s\n", ok ? "ok  " : "FAIL", what.c_str());
   if (!ok) ++g_fails;
}

void setVM(VoiceManager& vm, MIDIParamItem item, float v) {
   vm.setVoiceManagerParameter(MODULE_PARAM_ADDRESS(ModuleParamKindMIDI, ModuleInstance1, item), v);
}
void setBank(VoiceManager& vm, ModalBankParamItem item, float v) {
   vm.setParameter(MODULE_PARAM_ADDRESS(ModuleParamKindModalBank, ModuleInstance1, item), v);
}

// Poly, one Harmonic mode per voice, instant pitch slew (as the plugin sets it in poly).
void configure(VoiceManager& vm, bool mono = false) {
   setVM(vm, MIDIParamItemVoiceCount, mono ? 1.0f : 10.0f);
   setVM(vm, MIDIParamItemLegato, mono ? 1.0f : 0.0f);
   setVM(vm, MIDIParamItemChannelFilterValue, 0);
   setVM(vm, MIDIParamItemGroupFilterValue, 0);
   setVM(vm, MIDIParamItemVoiceStealAlgorithm, 0);
   setVM(vm, MIDIParamItemFreeRun, 0);
   vm.setParameter(slew1RiseAddress, 1e-8f);
   vm.setParameter(slew1FallAddress, 1e-8f);
   setBank(vm, ModalBankParamItemMaterial, float(ModalBankMaterialGen0));
   setBank(vm, ModalBankParamItemModeCount, 1.0f);
   setBank(vm, ModalBankParamItemDecayT60, 60.0f);
   setBank(vm, ModalBankParamItemSpread, 0.0f);
}

std::vector<float> render(VoiceManager& vm, uint32_t frames) {
   std::vector<float> out;
   out.reserve(frames);
   float L[kBlock], R[kBlock];
   while (out.size() < frames) {
      const uint32_t n = std::min<uint32_t>(kBlock, frames - uint32_t(out.size()));
      std::fill_n(L, n, 0.0f);
      std::fill_n(R, n, 0.0f);
      vm.process(n, L, R);
      out.insert(out.end(), L, L + n);
   }
   return out;
}

// Frequency from interpolated upward zero crossings over the settled part of the buffer.
double estimateFrequency(const std::vector<float>& s) {
   const size_t start = s.size() / 10;
   double first = -1.0, last = -1.0;
   int crossings = 0;
   for (size_t i = start + 1; i < s.size(); ++i) {
      if (s[i - 1] < 0.0f && s[i] >= 0.0f) {
         const double t = double(i - 1) + double(-s[i - 1]) / double(s[i] - s[i - 1]);
         if (first < 0.0) first = t; else ++crossings;
         last = t;
      }
   }
   return crossings > 0 ? kSampleRate * crossings / (last - first) : 0.0;
}

// Hann-windowed single-bin DFT magnitude over the settled part — tells which of several
// simultaneous notes are sounding, where a zero-crossing count cannot.
double toneLevel(const std::vector<float>& s, double hz) {
   const size_t start = s.size() / 4;
   const size_t n = s.size() - start;
   const double w = 2.0 * M_PI * hz / kSampleRate;
   double re = 0.0, im = 0.0;
   for (size_t i = 0; i < n; ++i) {
      const double hann = 0.5 - 0.5 * std::cos(2.0 * M_PI * double(i) / double(n - 1));
      const double x = double(s[start + i]) * hann;
      re += x * std::cos(w * double(i));
      im -= x * std::sin(w * double(i));
   }
   return 2.0 * std::sqrt(re * re + im * im) / (0.5 * double(n));
}

bool near(double measured, double expected) { return std::fabs(measured - expected) / expected < 0.001; }
double semitonesUp(double hz, double semis) { return hz * std::pow(2.0, semis / 12.0); }
std::string hz(double v) { return std::to_string(v) + " Hz"; }
std::string num(double v) { return std::to_string(v); }
std::string num(uint32_t v) { return std::to_string(v); }

// Any MIDI 2.0 channel-voice UMP; word0's low byte is the attribute type / per-note controller
// index / per-note management flags depending on the status.
pm::UmpMessage makeUmp(uint8_t status, uint8_t channel, uint8_t note, uint8_t lowByte, uint32_t word1) {
   const uint32_t word0 = (0x4u << 28) | (uint32_t(status & 0xF) << 20) | (uint32_t(channel & 0xF) << 16)
                        | (uint32_t(note) << 8) | uint32_t(lowByte);
   const uint32_t words[2] = {word0, word1};
   return pm::decodeUmp(words);
}
pm::UmpMessage noteOn(uint8_t note, uint8_t channel = 0) {
   return makeUmp(pm::kStatusNoteOn, channel, note, 0, 0xC000u << 16);
}
pm::UmpMessage noteOff(uint8_t note, uint8_t channel = 0) {
   return makeUmp(pm::kStatusNoteOff, channel, note, 0, 0);
}
uint32_t bendWord(double semitones, double range) {
   return uint32_t(std::llround(2147483648.0 + semitones / range * 2147483648.0));
}
pm::UmpMessage perNoteBend(uint8_t note, double semitones, uint8_t channel = 0, double range = 48.0) {
   return makeUmp(pm::kStatusPerNotePitchBend, channel, note, 0, bendWord(semitones, range));
}
// Registered controller bank 0 / index 7 (per-note pitch bend sensitivity), encoded as KeyboardEDO
// does: semitones in bits 31-25, cents in bits 24-18.
pm::UmpMessage perNoteRange(double semitones, uint8_t channel = 0) {
   const uint32_t whole = uint32_t(semitones);
   const uint32_t cents = uint32_t(std::lround((semitones - whole) * 100.0));
   return makeUmp(pm::kStatusRegController, channel, 0, 7, (whole << 25) | (cents << 18));
}

void testDecode() {
   std::printf("MIDI 2.0 per-note decode:\n");
   const pm::UmpMessage bend = makeUmp(pm::kStatusPerNotePitchBend, 3, 61, 0, 0xC0000000u);
   check(pm::isPerNoteMessage(bend) && bend.note == 61 && bend.channel == 3 && bend.data32 == 0xC0000000u,
         "per-note pitch bend keeps note, channel and 32-bit value");
   check(pm::pitchBendToUnit(0x80000000u) == 0.0 && pm::pitchBendToUnit(0xC0000000u) == 0.5
             && pm::pitchBendToUnit(0) == -1.0,
         "bend centre is exactly 0, a quarter of the range up is 0.5, the bottom is -1");
   const pm::UmpMessage rpnc = makeUmp(pm::kStatusRegPerNoteController, 0, 60, 74, 123u);
   check(pm::isPerNoteMessage(rpnc) && rpnc.ccIndex == 74 && rpnc.note == 60 && rpnc.ccData == 123u,
         "registered per-note controller: index from the low byte, note from the note byte");
   const pm::UmpMessage cc = makeUmp(pm::kStatusControlChange, 0, 74, 0, 5u);
   check(!pm::isPerNoteMessage(cc) && cc.ccIndex == 74, "control change still takes its index from the note byte");
   const pm::UmpMessage mgmt = makeUmp(pm::kStatusPerNoteManagement, 0, 60, pm::kPerNoteMgmtReset, 0);
   check(pm::isPerNoteMessage(mgmt) && mgmt.optionFlags == pm::kPerNoteMgmtReset,
         "per-note management carries its option flags");
   const pm::UmpMessage rc7 = perNoteRange(2.5, 4);
   check(rc7.status == pm::kStatusRegController && rc7.note == 0 && rc7.ccIndex == 7 && rc7.channel == 4
             && !pm::isPerNoteMessage(rc7),
         "registered controller: bank from the note byte, index from the low byte, channel-wide");
   const pm::UmpMessage m1bend = pm::fromMidi1Bytes(0xE2, 0x00, 0x40);
   check(m1bend.status == pm::kStatusPitchBend && m1bend.channel == 2 && m1bend.data32 == 0x80000000u,
         "MIDI 1.0 pitch bend decodes to a centred 32-bit bend");
}

void testUnbentPitch() {
   std::printf("Reference pitch (no bend):\n");
   VoiceManager vm(kSampleRate, kBlock);
   configure(vm);
   vm.handleMIDIMessage(noteOn(60));
   const double measured = estimateFrequency(render(vm, uint32_t(kSampleRate)));
   check(near(measured, pm::noteToFrequency(60)),
         "a single Harmonic mode rings at the note's fundamental (" + hz(measured) + ")");
}

void testChannelBend() {
   std::printf("Channel pitch bend:\n");
   VoiceManager vm(kSampleRate, kBlock);
   configure(vm);
   vm.handleMIDIMessage(pm::fromMidi1Bytes(0xE0, 0x00, 0x60));   // MIDI 1.0, half way up: +1 semitone
   vm.handleMIDIMessage(pm::fromMidi1Bytes(0x90, 60, 100));      // a bend sent before the note applies
   const double expected = semitonesUp(pm::noteToFrequency(60), 1.0);
   const double measured = estimateFrequency(render(vm, uint32_t(kSampleRate)));
   check(near(measured, expected),
         "+1 semitone channel bend at the default ±2 range (" + hz(measured) + " vs " + hz(expected) + ")");

   VoiceManager other(kSampleRate, kBlock);
   configure(other);
   other.handleMIDIMessage(pm::fromMidi1Bytes(0x90, 60, 100));
   other.handleMIDIMessage(pm::fromMidi1Bytes(0xE1, 0x7F, 0x7F));
   const double unbent = estimateFrequency(render(other, uint32_t(kSampleRate)));
   check(near(unbent, pm::noteToFrequency(60)), "a channel-2 bend leaves a channel-1 note alone (" + hz(unbent) + ")");

   // The bend reaches a note that is already ringing (de-zippered, so measure after it settles).
   VoiceManager live(kSampleRate, kBlock);
   configure(live);
   live.handleMIDIMessage(noteOn(60));
   render(live, 4096);
   live.handleMIDIMessage(makeUmp(pm::kStatusPitchBend, 0, 0, 0, bendWord(-2.0, 2.0)));
   const double down = estimateFrequency(render(live, uint32_t(kSampleRate)));
   check(near(down, semitonesUp(pm::noteToFrequency(60), -2.0)),
         "a MIDI 2.0 channel bend retunes a ringing note, full range down (" + hz(down) + ")");
}

void testPerNoteBend() {
   std::printf("MIDI 2.0 per-note pitch bend:\n");
   const double c4 = pm::noteToFrequency(60), g4 = pm::noteToFrequency(67);
   const double cSharp4 = semitonesUp(c4, 1.0);

   VoiceManager vm(kSampleRate, kBlock);
   configure(vm);
   vm.handleMIDIMessage(noteOn(60));
   vm.handleMIDIMessage(noteOn(67));
   vm.handleMIDIMessage(perNoteBend(60, 1.0));
   const std::vector<float> chord = render(vm, uint32_t(kSampleRate));
   const double bent = toneLevel(chord, cSharp4), old = toneLevel(chord, c4), other = toneLevel(chord, g4);
   check(bent > 0.01 && old < 0.1 * bent,
         "+1 semitone on C4 moves it to C#4 (C#4 " + num(bent) + ", C4 " + num(old) + ")");
   check(other > 0.5 * bent, "the other held note (G4) is untouched (" + num(other) + ")");

   // Two notes on the SAME key but different channels are different notes.
   VoiceManager same(kSampleRate, kBlock);
   configure(same);
   same.handleMIDIMessage(noteOn(60, 0));
   same.handleMIDIMessage(noteOn(60, 1));
   check(same.activeCount() == 2, "C4 on channels 1 and 2 takes two voices (got " + num(same.activeCount()) + ")");
   same.handleMIDIMessage(perNoteBend(60, 12.0, 1));
   const std::vector<float> pair = render(same, uint32_t(kSampleRate));
   const double c5 = pm::noteToFrequency(72);
   check(toneLevel(pair, c4) > 0.01 && toneLevel(pair, c5) > 0.01,
         "a per-note bend on channel 2's C4 leaves channel 1's C4 in place (C4 " + num(toneLevel(pair, c4))
             + ", C5 " + num(toneLevel(pair, c5)) + ")");
   // Short decay so a released note rings out quickly; the held one stays Active regardless.
   setBank(same, ModalBankParamItemDecayT60, 0.05f);
   same.handleMIDIMessage(noteOff(60, 1));
   render(same, uint32_t(kSampleRate));
   check(same.activeCount() == 1, "a note-off on channel 2 releases only channel 2's note (active="
                                      + num(same.activeCount()) + ")");

   // Per-note bend must not survive into the voice's next note.
   VoiceManager reuse(kSampleRate, kBlock);
   configure(reuse);
   setVM(reuse, MIDIParamItemVoiceCount, 1.0f);   // the next note lands on the same voice
   reuse.handleMIDIMessage(noteOn(60));
   reuse.handleMIDIMessage(perNoteBend(60, 2.0));
   render(reuse, 4096);
   reuse.handleMIDIMessage(noteOff(60));
   reuse.handleMIDIMessage(noteOn(60));
   const double fresh = estimateFrequency(render(reuse, uint32_t(kSampleRate)));
   check(near(fresh, c4), "per-note bend resets on the next note-on (" + hz(fresh) + ")");

   // Per-note management with the Reset flag recentres the bend.
   VoiceManager mgmt(kSampleRate, kBlock);
   configure(mgmt);
   mgmt.handleMIDIMessage(noteOn(60));
   mgmt.handleMIDIMessage(perNoteBend(60, 3.0));
   render(mgmt, 4096);
   mgmt.handleMIDIMessage(makeUmp(pm::kStatusPerNoteManagement, 0, 60, pm::kPerNoteMgmtReset, 0));
   const double reset = estimateFrequency(render(mgmt, uint32_t(kSampleRate)));
   check(near(reset, c4), "per-note management Reset recentres the bend (" + hz(reset) + ")");

   // Mono: a bend for a held-but-silent note in the stack must not move the sounding note.
   VoiceManager mono(kSampleRate, kBlock);
   configure(mono, true);
   mono.handleMIDIMessage(noteOn(60));
   mono.handleMIDIMessage(noteOn(64));
   mono.handleMIDIMessage(perNoteBend(60, 5.0));
   const double e4 = pm::noteToFrequency(64);
   const double sounding = estimateFrequency(render(mono, uint32_t(kSampleRate)));
   check(near(sounding, e4), "mono: bending the held-but-silent C4 leaves the sounding E4 alone (" + hz(sounding) + ")");
   mono.handleMIDIMessage(perNoteBend(64, -4.0));
   render(mono, 4096);
   const double bentE = estimateFrequency(render(mono, uint32_t(kSampleRate)));
   check(near(bentE, c4), "mono: bending the sounding E4 down 4 semitones (" + hz(bentE) + ")");
}

// MIDI 2.0 registered controller 7 sets the per-note bend range for its channel; a controller that
// announces ±2 and scales its bends to match must be heard at ±2, not the ±48 default.
void testPerNoteBendRange() {
   std::printf("Per-note pitch bend sensitivity (registered controller 7):\n");
   const double c4 = pm::noteToFrequency(60);

   VoiceManager vm(kSampleRate, kBlock);
   configure(vm);
   vm.handleMIDIMessage(perNoteRange(2.0));
   vm.handleMIDIMessage(noteOn(60));
   vm.handleMIDIMessage(perNoteBend(60, 1.0, 0, 2.0));   // half way up at ±2
   const double bent = estimateFrequency(render(vm, uint32_t(kSampleRate)));
   check(near(bent, semitonesUp(c4, 1.0)), "at ±2, a half-way per-note bend is +1 semitone (" + hz(bent) + ")");

   VoiceManager other(kSampleRate, kBlock);
   configure(other);
   other.handleMIDIMessage(perNoteRange(2.0, 5));        // a different channel
   other.handleMIDIMessage(noteOn(60));
   other.handleMIDIMessage(perNoteBend(60, 3.0));        // +3 of the ±48 default
   const double untouched = estimateFrequency(render(other, uint32_t(kSampleRate)));
   check(near(untouched, semitonesUp(c4, 3.0)),
         "a range set on channel 6 leaves channel 1 at the ±48 default (" + hz(untouched) + ")");

   VoiceManager cents(kSampleRate, kBlock);
   configure(cents);
   cents.handleMIDIMessage(perNoteRange(2.5));
   cents.handleMIDIMessage(noteOn(60));
   cents.handleMIDIMessage(makeUmp(pm::kStatusPerNotePitchBend, 0, 60, 0, 0xFFFFFFFFu));   // full up
   const double full = estimateFrequency(render(cents, uint32_t(kSampleRate)));
   check(std::fabs(full / c4 - std::pow(2.0, 2.5 / 12.0)) < 0.001,
         "the cents field counts: full scale at 2 semitones 50 cents (" + hz(full) + ")");

   VoiceManager kept(kSampleRate, kBlock);
   configure(kept);
   kept.handleMIDIMessage(perNoteRange(2.0));
   kept.reset();                                          // a host reset, e.g. on transport stop
   kept.handleMIDIMessage(noteOn(60));
   kept.handleMIDIMessage(perNoteBend(60, 1.0, 0, 2.0));
   const double afterReset = estimateFrequency(render(kept, uint32_t(kSampleRate)));
   check(near(afterReset, semitonesUp(c4, 1.0)), "the range survives a reset (" + hz(afterReset) + ")");
}

// Note-offs the engine generates itself (sustain pedal, all-notes-off, panic) must carry the
// note's channel, or they never match the note they are meant to release.
void testGeneratedNoteOffsMatchChannel() {
   std::printf("Engine note-offs on other channels:\n");
   VoiceManager pedal(kSampleRate, kBlock);
   configure(pedal);
   setBank(pedal, ModalBankParamItemDecayT60, 0.05f);
   pedal.handleMIDIMessage(pm::fromMidi1Bytes(0xB5, 64, 127));   // pedal down, channel 6
   pedal.handleMIDIMessage(pm::fromMidi1Bytes(0x95, 60, 100));
   render(pedal, 2048);
   pedal.handleMIDIMessage(pm::fromMidi1Bytes(0x85, 60, 0));     // deferred by the pedal
   render(pedal, uint32_t(kSampleRate / 2));
   check(pedal.activeCount() == 1, "the pedal holds a channel-6 note after its note-off");
   pedal.handleMIDIMessage(pm::fromMidi1Bytes(0xB5, 64, 0));
   render(pedal, uint32_t(kSampleRate));
   check(pedal.activeCount() == 0, "pedal up releases the channel-6 note (active=" + num(pedal.activeCount()) + ")");

   VoiceManager all(kSampleRate, kBlock);
   configure(all);
   setBank(all, ModalBankParamItemDecayT60, 0.05f);
   all.handleMIDIMessage(pm::fromMidi1Bytes(0x9A, 64, 100));
   render(all, 2048);
   all.allNotesOff();
   render(all, uint32_t(kSampleRate));
   check(all.activeCount() == 0, "all-notes-off releases a channel-11 note (active=" + num(all.activeCount()) + ")");

   VoiceManager panic(kSampleRate, kBlock);
   configure(panic);
   setBank(panic, ModalBankParamItemDecayT60, 0.05f);
   panic.handleMIDIMessage(makeUmp(pm::kStatusNoteOn, 7, 50, 0, 0xC000u << 16));
   panic.handleMIDIMessage(makeUmp(pm::kStatusNoteOn, 9, 72, 0, 0xC000u << 16));
   render(panic, 2048);
   panic.panic();
   render(panic, uint32_t(kSampleRate));
   check(panic.activeCount() == 0, "panic releases notes on channels 8 and 10 (active=" + num(panic.activeCount()) + ")");
}

} // namespace

int main() {
   std::printf("StruckSynth pitch-bend test (%.0f Hz)\n", kSampleRate);
   testDecode();
   testUnbentPitch();
   testChannelBend();
   testPerNoteBend();
   testPerNoteBendRange();
   testGeneratedNoteOffsMatchChannel();
   if (g_fails == 0) std::printf("ALL PASS\n");
   else std::printf("%d FAILED\n", g_fails);
   return g_fails ? 1 : 0;
}
