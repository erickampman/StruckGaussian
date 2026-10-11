# Testing StruckSynth

The project ships a suite of in-process self-tests (via CTest) plus the CLAP conformance
validator. Everything runs headless on Linux/macOS — no DAW or audio device required.

## Prerequisites

- CMake ≥ 3.17 and a C++20 compiler (gcc or clang).
- Submodules (CLAP headers for the tests; DPF + pugl for the plugin build):

  ```sh
  git submodule update --init --recursive
  ```

- To build the plugin itself (DPF's OpenGL/NanoVG UI), the X11 + GL dev packages:

  ```sh
  sudo apt install pkg-config libgl1-mesa-dev libx11-dev libxext-dev libxrandr-dev libxcursor-dev
  ```

## Build and run all tests

```sh
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Add `-DSTRUCKSYNTH_WERROR=ON` at configure time to treat warnings as errors for the framework
and the `pm-selftest` glue (this is what CI keys on). The tests that pull in vendored
PolygraphModular headers stay on that code's own warning style.

## What each test covers

Eleven CTest cases exercise the `pm_framework` engine end-to-end; `ss-clap` drives the built plugin
itself, and `clap-validator` (below) checks CLAP conformance:

| CTest name       | Executable               | Verifies |
|------------------|--------------------------|----------|
| `pm`             | `pm-selftest`            | UMP MIDI layer: predicates, raw-word decode, microtonal pitch attribute, CLAP→UMP decoders |
| `pm-module`      | `pm-module-test`         | `Module` + `ModuleMap` core: param read/write, process proc, map add/get |
| `pm-sinosc`      | `pm-sinosc-test`         | The vendored sine oscillator produces an ~A440 sine |
| `pm-ifscore`     | `pm-ifscore-test`        | IFSCore's per-point smoothed eval matches its wavetable (the IFS modules inherited from IFSynth) |
| `pm-adsr`        | `pm-adsr-test`           | ADSR envelope: note-on drives attack, note-off drives release to ~0 |
| `pm-modules`     | `pm-modules-test`        | Combine / Final / Slew compile+run; PitchConverter maps note→pitch |
| `pm-factory`     | `pm-factory-test`        | ModuleFactory + the init-specs build all 19 registered modules |
| `pm-voice`       | `pm-voice-test`          | One full voice renders a note end-to-end and releases |
| `pm-modroute`    | `pm-modroute-test`       | Modulation routing at the voice level: connecting a mod source switches the destination's path |
| `pm-voicemanager`| `pm-voicemanager-test`   | Polyphony: a 3-note chord allocates 3 voices and frees them on release |
| `ss-bend`        | `ss-bend-test`           | MIDI 2.0 per-note decode; channel and per-note pitch bend (pitch measured on a single-mode bank); note matching on note + channel + group; per-note bend range (RC 7); Pitch 7.25; CLAP note ids; CLAP note expression conversion; note lifetime (ring-out, mono stack, steal) for note end; pedal / all-notes-off / panic release notes on any channel |
| `ss-clap`        | `ss-clap-host-test`      | The built `.clap` over the CLAP C API: note-port dialects, params + state, 7.9-pitch notes, per-note and channel bend, RC 7, CLAP tuning expressions (with key, and by note id alone) reach the resonator; NOTE_END comes once, after the ring-out, for CLAP notes only; voice-info; pitch is measured to a fraction of a sample |

## Running a single test

```sh
ctest --test-dir build -R pm-voice --output-on-failure   # by name (regex)
./build/pm-voice-test                                     # or run the binary directly
```

Each test binary prints diagnostic values (e.g. `Voice: peak=0.87 tail=0.0000`) and ends with
`ALL PASS`; CTest keys success on that string.

## CLAP conformance

Validate the built plugin against the CLAP spec with
[`clap-validator`](https://github.com/free-audio/clap-validator):

```sh
clap-validator validate build/StruckSynth.clap
```

## Continuous integration

`.github/workflows/ci.yml` runs the configure + build + `ctest` on every push across
Linux (gcc/clang) and macOS (clang) with `-DSTRUCKSYNTH_WERROR=ON`.
