# StruckGaussian milestones

Cloned from StruckSynth (c59add0) on 2026-10-25; M1-M4 and the mode-cloud experiment below are StruckSynth's,
inherited with its history.

## M5: The mode cloud
The object is always a Gaussian cloud of modes (src/Module/ModalCloud.hpp, from tools/cloudlab, whose README has
the measurements). Decided 2026-10-25:
- Material is gone. Object panel: Pitch, Centre, Width, Modes, Seed, Reseed, Keep Fund, Stretch. Decay and Timbre as in
  StruckSynth. Unreleased, so the cloud's parameters sit in panel order rather than appended.
- Centre is octaves above the played note: 0 (the default) centres the cloud on the note. Width 0.002-3 octaves
  (log), default 0.5. Modes default 16.
- Seed (1-100) is stored in the preset: one seed is one object. Reseed Per Note draws a new cloud at every note-on
  (a counter shared by all voices, so chords differ too); Off plays the Seed's cloud, the same one cloudlab draws.
- Keep Fund (default Yes): mode 0 is the note's fundamental.
- Trim: the strike is scaled by Bar's in-phase peak at the defaults (2.1181) over the bank's, so Modes, Width and
  Centre don't change the level of the hit (cloudlab: 1.20 peak at 4, 16 and 64 modes, against 0.22-0.88 untrimmed;
  about 2 dB over Bar). It scales the input, so a moving trim never changes a ringing note.
- ModalBank gained modeAmplitude() for the trim (and later setPositionWeighting()); the other synths' copies don't
  have them yet.
- Loudness curve (Focus), replacing the damp bell's three knobs, which went from the Decay panel to Timbre as Focus,
  Focus Ctr, Focus Wid. The cloud's Gaussian places the modes; Focus shapes how loud they start. Focus morphs each
  mode's amplitude from 1/sqrt(ratio) (0, the default) to a Gaussian in octaves, peaked at Focus Ctr (default 0 = f0)
  with width Focus Wid (0.05-3 octaves, log, default 0.5): amp = (1/sqrt r)^(1-F) * gauss^F. The damp bell stays
  in ModalBank, off.
- The spectrum's bar height is each mode's starting amplitude (ModalBank::modeAmplitude); opacity is its decay time.
  Its axis is centred on f0 and reaches the furthest mode either side, so the f0 line stays put as Width moves.
- Isolation (2026-10-25, after the pitch was heard moving with Width):
  - Clouds are recentred: the drawn values are shifted to average 0, so the mean log-ratio is exactly Centre for
    every seed, and Width spreads modes symmetrically about it. Raw draws had drifted the cloud off Centre in
    proportion to Width (seed 3, 25 modes: -103 cents at Width 0.2). A recentred draw outside 1/16..120 is dropped.
    Clouds no longer match the StruckSynth cloudlab renders.
  - Strike Pos, Brightness and Tmb>Pos removed. Both knobs moved the loudness peak, Focus's job: Strike Position's
    |sin(pi*ratio*pos)| assumes a string's mode shapes, which a cloud lacks (near 0 it is a steep upward tilt).
    ModalBank gained setPositionWeighting(), off for the cloud; Brightness stays at 0 in the engine.
  - With Keep Fund on and Focus Ctr 0, f0 is the loudest mode at every Width (seed 34, Width 0.2-1). With it off,
    the loudest mode is the draw nearest the Focus peak, which moves away as Width grows (+19 to +93 cents).
- [x] Cloud generator, host parameters, panel, Init preset; the spectrum shows the Seed's cloud with a line at the
      fundamental (f0), since a cloud has modes below it too
- [x] ss-bend: Seed / Reseed test; the bend and expression tests now use the cloud's kept fundamental
- [x] ctest 12/12; clap-validator 35 pass, 0 fail
- [x] By hand: Object (eight controls), Timbre (three: Focus) and Expression (two) checked by eye (2026-10-25)
- [x] By hand: played in StageHand (2026-10-25)
- [x] Presets: StruckSynth's four material presets replaced by eight cloud presets (2026-10-25), designed from the
      cloudlab measurements, one per kind of sound: Chorus Unison (a unison-narrow cloud on f0), Octave Shimmer (f0 and
      a chorused octave), Focused Glass (wide cloud, narrow Focus on f0), Halo (48 modes, a broad Focus), Dark Gong
      (centred half an octave under, long low modes), Clang Cluster (1.5 octaves up, bright), Wood Cluster (10 modes,
      short and damped), Never Twice (Reseed Per Note). Rendered through cloudlab at C2/C4/C6: peaks within 2.6 dB of
      each other; Dark Gong's T60 5-6 s after narrowing (the first draft rang 18 s, low modes to 38 s)
- [x] By hand: presets listened to (2026-10-25)
- [x] Help page and README describe the cloud (2026-10-25): Object, Timbre (Focus), Decay (modes below the note
      ring longer), the spectrum, the presets, tips and troubleshooting. help.html regenerated from help.md, with
      real tables (the old converter left them as text)
- [x] A strike's first block was wrong, two bugs inherited from StruckSynth (same probe there):
      - The bank read its pitch at the block's first sample, where the Slew (0.21 oct a sample at its fastest) has
        barely left the voice's previous pitch: the whole first block rang at the old note, then retuned (C5 after C4
        started near 281 Hz; a voice's first note started from A440). It reads the block's last sample now.
      - Final's level smoother started every voice at 1, not Level, so a voice's first note was 1/Level louder for
        ~20 ms (+3 dB at the default 0.7). It starts at the first block's values now.
      ss-bend checks both. The other synths have one or both: ReleaseUMP R2-12, R2-13
- [ ] macOS: build, auval (aumu StGa nLKL)

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

## M3: VST3 / AUv2 via clap-wrapper
- [x] VST3 (macOS + Linux) and AUv2 (macOS, `aumu StSy nLKL`) through clap-wrapper v0.16.0, wrapping the same CLAP
- [x] VST3 note expressions: Tuning + Brightness offered via `CLAP_PLUGIN_AS_VST3` (Pressure as before)
- [x] clap-wrapper AUv2 GUI patch carried in `patches/` (calls `gui->show()`, which starts the render timer)
- [x] Validated on macOS (Xcode 26.3, Ninja, 2026-10-21): ctest 12/12; clap-validator 35 pass, 0 fail;
      auval passes; Steinberg's VST3 validator 47 pass, 0 fail (lists Tuning + Brightness)
- [ ] By hand: GUI, notes, automation and state recall in Logic (AUv2), REAPER (VST3 + AUv2), Bitwig (VST3);
      Bitwig per-note Tuning in the VST3
- [ ] Linux: VST3 build in CI; not yet loaded in a Linux host

## M4: Later
- [x] Pitch 7.25 as persistent per-key tuning table (CLAP tuning stays per-note)
- [x] Move onto the DPF fork (erickampman/DPF) with own DGL_NAMESPACE (2026-10-23)

## Experiment: mode cloud (2026-10-25)
- [x] `tools/cloudlab`: offline renderer, the object's modes drawn from a Gaussian (log2 ratio ~ Normal(mean, sigma)
      in octaves, fundamental kept by default, amplitude 1/sqrt(ratio), seeded and the same on every platform),
      fed to the plugin's own ModalBank. Sweeps of sigma, mean, count and seed at C2/C4/C6, measured; decisions and
      findings in `tools/cloudlab/README.md`. No plugin, parameter or UI change.
- [x] Listened to (2026-10-25): worth keeping, as its own instrument rather than a series in the other synths'
      ModalGenerators.hpp. That is StruckGaussian (M5 above); the generator is `src/Module/ModalCloud.hpp`
