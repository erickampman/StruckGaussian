# StruckSynth

A cross-platform **CLAP modal-synthesis instrument** from Unlikelyware — a struck-resonator
voice built on the PolygraphModular voice engine. Each note fires a shaped excitation burst
into a bank of tuned resonant modes, so the sound is a struck object (bell, bar, tine, block)
rather than an oscillator through a filter. Scaffolded from IFSynth, it carries the same
vendored PolygraphModular engine and DPF/NanoVG UI kit.

## Status

**Polyphonic struck-resonator synth with a full custom GUI.** The PM framework drives a
`VoiceManager` (polyphony, note-stealing, mono/legato) rendering the voice
**Strike → ModalBank → Final**, with pitch tracking via **PitchConverter → Slew** (glide) into
the bank's pitch input. There is no VCA envelope: the bank's own T60 decay is the amplitude
envelope, and a voice frees itself once it rings down to silence, so the tail survives
note-off.

- **Strike** (exciter): Mallet or Noise burst, hardness, velocity → level / hardness, contact
  mode.
- **ModalBank** (resonator): up to 64 modes, material presets, stretch, decay (T60), damping
  tilt + bell, strike position, brightness, stereo spread.
- **Voices**: Poly (10 voices) or Mono (legato, with glide).
- **Presets**: factory presets in `Presets/*.json`, baked into the plugin at build time.

MIDI notes are sample-accurate: `process()` splits rendering at each event's frame offset.
Parameter changes are applied per host block, and the note port advertises (and prefers) the MIDI
2.0 dialect, so a 7.9-pitch NoteOn reaches the resonator unchanged.

### Pitch bend: channel and per-note

Every voice has a `PitchBendConverter` (`src/Module/PitchBendConverterModule.cpp`, ported from
PlainOldSynth) feeding ModalBank's second pitch input. That input comes **after** the Slew, so glide
never smears a bend, and it is scaled on its own, so a fixed-pitch body (Pitch Mod Level 0) still
bends. It sums two bends:

- **Channel pitch bend** (MIDI 1.0 or 2.0), **±2 semitones**. Each voice remembers the latest bend of
  every channel and applies the one its own note was played on: ordinary pitch bend with everything on
  one channel, and per-note bend when each note has its own channel (MPE-style). MPE's master-channel
  bend is not implemented yet.
- **MIDI 2.0 per-note pitch bend** (UMP status `0x6`). It bends only the note it names, over the same
  range as its channel's bend (**±2** by default): M2-104 v1.0 says per-note bend "acts like Pitch
  Bend in every way" and gives it no range of its own. A controller can set a separate per-note range
  per channel with MIDI 2.0 **registered controller 7** (per-note pitch bend sensitivity, from a later
  revision of M2-104), as KeyboardEDO does; that range survives a host `reset()`, since it is
  configuration the controller sends once.

Bend changes ramp over 2 ms, and a note starts at its bent pitch. ModalBank retunes its modes once per
block without resetting them, so a bend glides the ringing tail rather than re-striking it. Per-note
bend resets on every note-on and on a per-note management Reset; one sent *before* its note-on is
dropped, because no voice owns that note yet. The default range is not a host parameter yet, and
RPN 0 (channel bend range) is not handled yet.

**A note is note number + channel + group.** The same key on two channels is two voices with
independent per-note state, and note-offs the engine generates itself (sustain-pedal release,
all-notes-off, panic) are built from the voice's own note-on so they match. In mono, per-note
expression for a held-but-silent note in the stack doesn't move the sounding note. When the host
tags notes with CLAP note ids, the ids decide instead, so a re-struck key is a new note.

### Pitch 7.25 and CLAP note expressions

**Pitch 7.25** (MIDI 2.0 registered per-note controller #3) sets an absolute pitch for the sounding
note, and both bends offset from it. Like the bends it bypasses the glide. KeyboardEDO's glide mode
sends it. The spec also makes it persistent, a tuning table that later notes on the same key
inherit; that is not implemented: it lasts for the sounding note and resets on the next note-on.

**CLAP note expressions** (`CLAP_EVENT_NOTE_EXPRESSION`, how Bitwig sends per-note expression) are
converted to MIDI 2.0 per-note messages:

- **Tuning** becomes Pitch 7.25 at key + tuning, so it is exact at any size within Pitch 7.25's
  0–128 semitone span rather than clamped to the ±2 bend range.
- **Pressure** becomes poly pressure and **Brightness** registered per-note controller #74. Nothing
  in StruckSynth responds to either yet; they are routed so a future modulation source can.
- Volume, Pan, Vibrato and Expression are ignored.

An expression may name only a note id: tuning then looks up that note's key. VST3 note expression
(PlainOldSynth's clap-wrapper extension) does not apply: StruckSynth has no VST3 build.

The [DISTRHO Plugin Framework (DPF)](https://github.com/DISTRHO/DPF) is used **only as the GUI
layer** (UIExporter + NanoVG + pugl). The CLAP entry point is hand-written in `src/plugin.cpp`,
because DPF's generated CLAP wrapper drops `CLAP_EVENT_MIDI2` and advertises only the MIDI/MPE note
dialects, which would make MIDI 2.0 7.9-pitch notes unreachable. This is the same raw-CLAP shape
PlainOldSynth uses.

## Layout

The engine (`src/`) mirrors PolygraphModular's structure; the CLAP plugin + GUI live in `src/`:

```
plugin/               the shared plugin definition
                        DistrhoPluginInfo.h (UI flags; the plugin identity macros are inert),
                        StruckSynthParams.h (shared param table + dropdown labels)
src/plugin.cpp        the raw CLAP plugin: entry point, note/audio/params/state/gui extensions,
                        and the DSP glue (MIDI + params → VoiceManager)
src/plugin_gui.h      the C seam between plugin.cpp and the GUI shell
src/UI/               the plugin GUI (reusable NanoVG widgets + the panel)
                        Knob, Slider, Toggle, Dropdown, PopupMenu, TabBar, GroupBox, TitleText,
                        Logo; PresetKeys / PresetDump (preset menu + dev-only JSON dumper);
                        StruckSynthUI.cpp — composes the controls into labelled groups and hosts
                        them through DPF's UIExporter
src/Common/           shim + MIDI + params + utilities
                        pm_compat.h (Apple→portable), pm_midi.* (spec UMP + MIDI-1 decode),
                        clap_ump.h (CLAP↔UMP), pm_params.h / pm_param_data.* / pm_addresses.h,
                        ParamAddressMacros.h, CPPTypes.hpp, CPPProcs.hpp, KPBuffPairHelper.hpp
src/Module/           Module core + modules
                        StrikeModule.*, ModalBank.* / ModalBankModule.* (+ ModalMaterials,
                        ModalGenerators, ModalDiscretizer), Slew, PitchConverter, Final; the
                        IFS / Sine / ADSR / LPF / CVMixer / Level / Combine modules inherited
                        from IFSynth stay instantiated but unconnected
src/Voice/            voice engine
                        Voice.*, VoiceStateManager.*, VoiceManager.*, ModuleMap.*, ModuleGraph.hpp,
                        ModuleInitSpecs.hpp, pm_module_specs.cpp, ConnectionCommandQueue.hpp
Presets/              factory presets (JSON), compiled into GeneratedPresets.h by
                        tools/gen_presets.py
Resources/            font + logo, embedded into the binary at build time
test/                 in-process self-tests (see TESTING.md)
dpf/                  DISTRHO Plugin Framework (submodule, + nested pugl) — generates the .clap
external/clap         CLAP headers (submodule, pinned 1.2.9) — used by the framework self-tests
```

Vendored PolygraphModular files keep their original `#include "X.hpp"` form; every framework
folder is on the include path so they resolve across `Common/` `Module/` `Voice/`. The DSP
wrapper links the `pm_framework` static lib; DPF owns the plugin/UI lifecycle.

## Build

DPF's OpenGL/NanoVG UI needs the usual X11 + GL dev packages (Linux):

```sh
sudo apt install pkg-config libgl1-mesa-dev libx11-dev libxext-dev libxrandr-dev libxcursor-dev
```

Then:

```sh
git clone --recurse-submodules https://github.com/erickampman/StruckSynth.git
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure     # see TESTING.md for the full suite
```

Produces `build/StruckSynth.clap` (a bundle on macOS). `cmake --build build --target install-clap`
symlinks it into `~/.clap` (Linux) or `~/Library/Audio/Plug-Ins/CLAP/` (macOS), so any CLAP host
picks up the freshest build after a rescan. Validate with
[`clap-validator`](https://github.com/free-audio/clap-validator). Test details in
[TESTING.md](TESTING.md).

Configure options: `-DSTRUCKSYNTH_WERROR=ON` (warnings as errors), `-DSTRUCKSYNTH_PRESET_DUMP=ON`
(dev-only DUMP button that writes the current patch as JSON to `$HOME`).

## License

MIT.
