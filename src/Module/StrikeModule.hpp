//
//  StrikeModule.hpp
//
//  Turns note events into a shaped excitation burst for a resonator (ModalBank,
//  RezNotch, a filter…). Folds the common "struck voice" chain — Note Gate +
//  Delta Impulse + Combine(velocity) — into one module, and adds the thing a
//  bare impulse cannot give: mallet HARDNESS.
//
//  Driven by MIDI note-on directly (velocity → strike strength) so no Note Gate
//  or Velocity Converter is needed. An optional Gate input takes over for
//  non-MIDI triggering (LFO / Sequencer / Clock); when it is connected, MIDI
//  triggering is suppressed so the two can't double-fire.
//

#ifndef StrikeModule_hpp
#define StrikeModule_hpp

#include "Module.hpp"

extern const void        InitStrikeModule(uint32_t count, Module* module);
extern const void        DeinitStrikeModule(Module* module);
extern const void        ResetStrikeModule(Module* module);
extern const void        SampleRateStrikeModule(Module* module, double newSampleRate);
extern void              MIDIStrikeModule(const pm::UmpMessage& message, Module* module);
extern const KPBuffPair* ProcessStrikeModule(uint64_t processCallID, uint32_t count, Module* module);

#endif /* StrikeModule_hpp */
