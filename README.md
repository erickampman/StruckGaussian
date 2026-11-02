# StruckGaussian

A struck object whose modes are a Gaussian cloud around the note: from a chorus on one pitch to a
wide clang. It is [StruckSynth](https://github.com/erickampman/StruckSynth) with its material
tables replaced by a random draw: the modes' pitches are scattered on a bell curve around a centre
you set.

![StruckGaussian's editor](docs/screenshot.png)

| | |
|---|---|
| Formats | CLAP. VST® 3 and AUv2 (macOS) build from source; ready-built in a later preview |
| Platforms | Linux x86-64, macOS 26 or later on Apple Silicon. No Windows |
| Version | 0.9.0 (preview) |
| License | MIT |

**Downloads:** ready-built plugins will be on itch.io with the 0.9 preview. This repo has the
source only.

## What makes it different

- **One knob from chorus to clang.** Width spreads the modes: below about 0.02 octave the cloud is a
  chorus on one pitch, around 0.5 a struck object, above 1 a wide clang. Centre moves the cloud
  above the note, and Focus shapes which modes are loudest.
- **Repeatable or new on every note.** A seed picks the cloud, so a preset always plays the same
  object on every platform, or Reseed draws a new one for every note.
- **MIDI 2.0 first.** The plugin takes MIDI 2.0 notes directly, so a Pitch 7.9 note plays its exact
  microtonal pitch, not the nearest semitone: an EDO keyboard such as
  [KeyboardEDO](https://github.com/erickampman/KeyboardEDO) is heard as played. Per-note pitch bend,
  Pitch 7.25 and per-note controllers reach only the note they name. MIDI 1.0 works too.
- **MPE and CLAP note expressions.** An MPE switch for MPE controllers, and in hosts that send CLAP
  notes (Bitwig) tuning, pressure and brightness arrive per note.

The [help page](Resources/help.md), also behind the editor's Help button, describes every control.
[docs/how-it-works.md](docs/how-it-works.md) has the cloud, the voice and the MIDI 2.0 details.

## Installing

The download holds `StruckGaussian.clap`. Put it in your CLAP folder (create the folder if it isn't there),
then rescan plugins in your host:

| | Linux | macOS |
|---|---|---|
| For you | `~/.clap` | `~/Library/Audio/Plug-Ins/CLAP` |
| For every user | `/usr/lib/clap` | `/Library/Audio/Plug-Ins/CLAP` |

On macOS, `~/Library` is hidden in Finder: Go ▸ Go to Folder opens it. The 0.9 previews aren't
notarized, so macOS won't load a downloaded plugin until its quarantine flag is cleared:

```bash
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/CLAP/StruckGaussian.clap
```

A host that is already running keeps the plugins it loaded: quit and restart it. To build StruckGaussian
yourself instead, see below.

## Building from source

```bash
git clone https://github.com/erickampman/StruckGaussian.git
cd StruckGaussian
tools/fetch_deps.sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

[BUILDING.md](BUILDING.md) has what to install first on Linux and macOS, the tests, installing
the plugin for hosts, and the build options.

## Help and bugs

Questions and bug reports go to [GitHub Issues](https://github.com/erickampman/StruckGaussian/issues).

## License

MIT. See [LICENSE](LICENSE) and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Trademarks

VST is a registered trademark of Steinberg Media Technologies GmbH. Audio Units, Logic, Mac and
macOS are trademarks of Apple Inc., registered in the U.S. and other countries. Linux is the
registered trademark of Linus Torvalds in the U.S. and other countries. Other product names are
trademarks of their owners, named here only to say what works with what.
