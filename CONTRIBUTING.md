# Contributing to StruckGaussian

Bug reports and pull requests are welcome. Build steps are in the README; this page is the rules the
code depends on.

## Before each commit

```sh
ctest --test-dir build --output-on-failure
clap-validator validate build/StruckGaussian.clap
```

The tests are the `pm-*` engine tests, `ss-bend` and `ss-clap`. `ss-clap` loads the built `.clap` and plays it headless, sending MIDI 2.0, MPE and CLAP events itself, so it needs no MIDI hardware. They are silent. On macOS, if you touched the AUv2 build, also run
`ninja -C build install-auv2` and `auval -v aumu StGa nLKL`.

After rebuilding, quit and restart the host before listening: a running host keeps the plugin binary
it loaded first.

## Parameters: append, never insert

The CLAP parameter id is the parameter's index, and hosts save sessions and automation by id. So a
new parameter goes at the **end** of the list, and existing ones are never reordered or removed: that
would silently rewire everyone's saved projects. For each new parameter:
- add it to the `symbolIs` order guard in `plugin/StruckGaussianParams.h`;
- if it shapes the sound, give it a preset key (`src/UI/PresetKeys.hpp`) and add a line for it to every `Presets/*.json`, edited as text so the files aren't reformatted.

MIDI controller setup (MPE on/off and the like) stays out of presets.

The cloud's parameters (Centre, Width, Seed, Reseed, Keep Fund, Focus, Focus Ctr, Focus Wid) were put
in panel order, and Material, Strike Pos, Brightness, Tmb>Pos and the damp bell's knobs removed, before
StruckGaussian's first release; no saved project had them yet. From the first release on, append.

## MIDI 2.0 and MPE

MIDI 2.0 per-note messages and CLAP note expressions are the main path. MPE is accepted as a
compatibility input (an Off/On switch, lower zone); nothing in this family of projects sends MPE.

## Threads

`state_load` runs on the main thread while `process()` may be running. It stores the loaded values
only; the audio thread applies them at the start of the next block (or `activate()` does). Never
rebuild the voice graph from the main thread.

## The mode cloud

`src/Module/ModalCloud.hpp` is a framework-free generator, like ModalGenerators; `ModalBankModule` feeds
its modes to ModalBank as a generated material (a stable buffer, `setMaterial`, `invalidate`).
- **A seed is the same cloud everywhere.** The Gaussian is Box-Muller on `std::mt19937`, whose output
  the standard fixes. Never `std::normal_distribution`: libc++ and libstdc++ draw different values, so a
  preset would be a different object on the Mac.
- **Recentred.** The draws are shifted to average exactly 0, so Width spreads the cloud about Centre
  without moving it. A recentred draw outside 1/16..120 is dropped, never redrawn or clamped.
- **Regenerated only when an input changes** (and, with Reseed, at a note-on), then `invalidate()`:
  the buffer pointer never changes, so `setMaterial` alone can't see new contents.
- **The trim scales the strike, not the output.** `kTrimRef` over the bank's in-phase peak
  (`ModalBank::modeAmplitude`), so Modes, Width and Focus leave the hit's level alone, and a trim that
  moves never changes a ringing note.
- **This ModalBank differs from the other synths' copies**: `modeAmplitude()` and
  `setPositionWeighting()` (off here: Strike Position's law assumes a string's mode shapes). Copy them
  across if the cloud goes to another synth.
- `tools/cloudlab/` is the offline renderer the cloud was prototyped in, with its README of findings;
  it builds `ModalCloud.hpp` and `ModalBank.cpp` from `src/Module` in place.

## Dependencies

The submodules are pinned (`tools/fetch_deps.sh`). Don't commit changes inside them. A fix needed in
clap-wrapper goes in `patches/` as a patch file, which `fetch_deps.sh` applies, so `external/clap-wrapper`
showing a local change is expected. DPF and pugl are Unlikelyware's forks; changes go to those repos
first and come here as a new pin.

## Drawing (NanoVG)

NanoVG fills non-convex paths through the stencil buffer and assumes it starts at zero, but DPF clears
only colour and depth. On Linux a stale stencil shows as speckles. Any widget that fills a non-convex
shape (a filled waveform, a polygon) must set `glStencilMask(0xff)` and clear the stencil before
drawing. No widget here fills a non-convex shape today. Convex fills (rectangles, rounded rectangles, circles) and strokes are
unaffected.

## Rules learned the hard way

- **Panels before their controls.** Declare each `GroupBox` member before the controls it parents.
  Members are destroyed in reverse order, so a panel freed first crashes its children's destructors
  when the editor closes. The host test only calls the GUI's `create()`, so only a real host catches
  this.
- **Every sound parameter in every preset.** The preset loader leaves keys a preset doesn't mention
  alone, so a missing key keeps the previous preset's value.
- **Output events stay in time order** within a block.
- **Controller setup survives `reset()`.** Configuration a controller sends once (RPN 0, registered
  controller 7) must outlive a reset: some hosts reset on transport stop.
- **Test helpers:** a full-scale bend computes 2³², which wraps to 0 in a `uint32_t`; clamp it.
- **Pitch tests:** measure on the simplest waveform you can. Zero crossings with interpolation are
  exact; autocorrelation needs peak interpolation.
- **A control input is read at the block's last sample.** The bank sets its pitch once per block, and a
  new note's pitch comes through the Slew, which needs a few samples even when instant: read at sample
  0, the whole first block rang at the voice's previous pitch.
- **Smoothers start at their target.** Final's level smoother once started each voice at 1, so a
  voice's first note was 1/Level louder for ~20 ms.

## Platform-specific code

- `src/UI/BackingScale_mac.mm` (Retina scale) and the AUv2 target are macOS-only.
- There is no VCA: the modal bank's T60 decay is the envelope, so a voice rings on after note-off and sends `CLAP_EVENT_NOTE_END` when it falls silent.

## Layout

The engine (`src/`) mirrors PolygraphModular's structure; the CLAP plugin + GUI live in `src/`:

```
plugin/               the shared plugin definition
                        DistrhoPluginInfo.h (UI flags; the plugin identity macros are inert),
                        StruckGaussianParams.h (shared param table + dropdown labels)
src/plugin.cpp        the raw CLAP plugin: entry point, note/audio/params/state/gui extensions,
                        and the DSP glue (MIDI + params → VoiceManager)
src/plugin_gui.h      the C seam between plugin.cpp and the GUI shell
src/UI/               the plugin GUI (reusable NanoVG widgets + the panel)
                        Knob, Slider, Toggle, Dropdown, PopupMenu, TabBar, GroupBox, TitleText,
                        Logo; PresetKeys / PresetDump (preset menu + dev-only JSON dumper);
                        StruckGaussianUI.cpp — composes the controls into labelled groups and hosts
                        them through DPF's UIExporter
src/Common/           shim + MIDI + params + utilities
                        pm_compat.h (Apple→portable), pm_midi.* (spec UMP + MIDI-1 decode),
                        clap_ump.h (CLAP↔UMP), pm_params.h / pm_param_data.* / pm_addresses.h,
                        ParamAddressMacros.h, CPPTypes.hpp, CPPProcs.hpp, KPBuffPairHelper.hpp
src/Module/           Module core + modules
                        StrikeModule.*, ModalBank.* / ModalBankModule.* (+ ModalCloud, the mode
                        cloud; ModalMaterials, ModalGenerators, ModalDiscretizer), Slew,
                        PitchConverter, Final; the
                        IFS / Sine / ADSR / LPF / CVMixer / Level / Combine modules inherited
                        from IFSynth stay instantiated but unconnected
src/Voice/            voice engine
                        Voice.*, VoiceStateManager.*, VoiceManager.*, ModuleMap.*, ModuleGraph.hpp,
                        ModuleInitSpecs.hpp, pm_module_specs.cpp, ConnectionCommandQueue.hpp
Presets/              factory presets (JSON), compiled into GeneratedPresets.h by
                        tools/gen_presets.py
Resources/            font + logo + help page (help.md, the source of help.html), embedded into the
                        binary at build time
tools/                fetch_deps.sh, apply_dep_patches.sh, gen_presets.py; cloudlab/ (the offline
                        renderer the mode cloud was prototyped in)
test/                 in-process self-tests (see TESTING.md)
dpf/                  DISTRHO Plugin Framework (submodule, + nested pugl) — the GUI layer only
external/clap         CLAP headers (submodule, pinned 1.2.9) — the plugin and the tests
external/clap-wrapper free-audio's clap-wrapper (submodule, v0.16.0) — the VST3 and AUv2 builds
external/vst3sdk      Steinberg VST3 SDK (submodule; only base, cmake, pluginterfaces, public.sdk)
patches/              local patches to the submodules, applied by tools/apply_dep_patches.sh
```

Vendored PolygraphModular files keep their original `#include "X.hpp"` form; every framework
folder is on the include path so they resolve across `Common/` `Module/` `Voice/`. The plugin
links the `pm_framework` static lib; DPF's `UIExporter` hosts the editor inside the CLAP gui
extension.
