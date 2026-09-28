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

MIDI notes are sample-accurate: `run()` splits rendering at each event's frame offset.
Parameter changes are applied per host block (DPF applies them before `run()`).

The wrapper is built with the [DISTRHO Plugin Framework (DPF)](https://github.com/DISTRHO/DPF),
which generates the CLAP around our DSP + UI subclasses.

## Layout

The engine (`src/`) mirrors PolygraphModular's structure; the DPF plugin assembly lives in
`plugin/` + `src/UI/`:

```
plugin/               the DPF plugin definition (the CLAP wrapper is generated from these)
                        DistrhoPluginInfo.h (plugin flags), StruckSynthParams.h (shared param
                        table), StruckSynthPlugin.cpp (DSP: MIDI + params → VoiceManager)
src/UI/               the plugin GUI (reusable NanoVG widgets + the panel)
                        Knob, Slider, Toggle, Dropdown, PopupMenu, TabBar, GroupBox, TitleText,
                        Logo; PresetKeys / PresetDump (preset menu + dev-only JSON dumper);
                        StruckSynthUI.cpp — composes the controls into labelled groups
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

Produces `build/bin/StruckSynth.clap`. A post-build step symlinks it into `~/.clap` (Linux) or
`~/Library/Audio/Plug-Ins/CLAP/` (macOS), so any CLAP host picks up the freshest build after a
rescan. Validate with [`clap-validator`](https://github.com/free-audio/clap-validator). Test
details in [TESTING.md](TESTING.md).

Configure options: `-DSTRUCKSYNTH_WERROR=ON` (warnings as errors), `-DSTRUCKSYNTH_PRESET_DUMP=ON`
(dev-only DUMP button that writes the current patch as JSON to `$HOME`).

## License

MIT.
