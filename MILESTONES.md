# StruckSynth milestones

## Tests
- all: cmake --build build -j && ctest --test-dir build --output-on-failure

## M1: Core struck-resonator synth
- [x] Strike exciter: Mallet / Noise burst, hardness, velocity → level / hardness, contact mode
- [x] ModalBank: up to 64 modes, material presets, stretch, T60 decay, damp tilt+bell, strike position, brightness, stereo spread
- [x] PitchConverter + Slew (glide); poly/mono with legato note stack
- [x] Expression panel: Pressure→Decay, Timbre→Damp Tilt, Timbre→Strike Position
- [x] Factory presets baked in; DPF/NanoVG UI

## M2: MIDI 2.0 and MPE port
- [x] Raw CLAP entry point (DPF as GUI only, MIDI 2.0 dialect available)
- [x] Per-note UMP decode; note matching on note + channel + group; RC 7 bend range; Pitch 7.25
- [x] CLAP note ids; note expressions (tuning as Pitch 7.25; pressure/brightness decoded)
- [x] `CLAP_EVENT_NOTE_END` after ring-out; voice-info extension
- [x] MPE lower zone: RPN 0 bend range, released-note control freeze, MPE on/off switch
- [x] Pressure and Timbre (CC 74) as per-note modulation sources
- [x] MIDI 1.0→2.0 Min-Center-Max upscale; MIDI 2.0 velocity-0 note-on treated as note-on
- [x] All Notes Off (CC 123) and All Sound Off (CC 120), per channel
- [x] Linux CI (Ubuntu gcc + clang with -Werror)

## M3: Later
- [ ] Pitch 7.25 as persistent per-key tuning table (currently applies to sounding note only)
- [ ] Move onto the DPF fork (erickampman/DPF) with own DGL_NAMESPACE
