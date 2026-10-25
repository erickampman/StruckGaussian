# cloudlab

An offline experiment (2026-10-25): what happens if a struck object's modes are a **Gaussian
cloud** instead of a series? Frequencies are drawn at random around a centre ratio, with a spread
in octaves. Nothing in the plugin changes. If the cloud is worth keeping it becomes a
`ModalGenerators.hpp` series, and that file gets copied to StruckKit, StruckRoom and
PolygraphModularUnified, which carry identical copies of `ModalBank`.

Not to be confused with `ModalBank`'s damp bell, which is a Gaussian over **decay times** (a band
of modes rings longer or shorter). The cloud is a Gaussian over **frequencies**.

`cloudlab` renders one struck note and writes a float WAV. The object is the plugin's own
`ModalBank`, compiled from `../../src/Module` in place, and fed the cloud as `ModalBankModule` feeds
a generator: a stable `ModalMode` buffer, `setMaterial`, `invalidate`. The strike is StrikeModule's
note-relative Hann contact. Everything else is at the plugin's defaults: T60 2 s, Damp Tilt 0.9,
Strike Position 0.25, Brightness 0, Spread 0.3, and Hardness 0.3 at velocity 1 (effective 0.15).

In StruckGaussian the cloud is the plugin's object: the generator is `src/Module/ModalCloud.hpp`, which
`cloudlab` now uses too, and the plugin trims the strike by its in-phase peak (`trim=1` here). Since
then the generator also recentres each cloud on its centre (Width no longer moves it), clouds are
rendered without strike-position weighting (`pos` acts on `mat=` tables only), and `focus`, `fctr`
and `fwid` set the loudness curve. The renders and measurements below were made in StruckSynth
before all of that, so today's clouds differ from them.

## Build and run

From this folder:

```bash
cmake -S . -B build -G Ninja && ninja -C build
```

```bash
python3 -I experiments.py
```

That renders every sweep into `renders/<experiment>/`, about 45 s and 180 MB in all. Each sweep gets
24-bit WAVs (`<variant>_<note>.wav`), a `spectrograms.png` with a column per note, and `stats.txt`
with the figures below plus the modes the bank built and the cloud's ratios. Give `experiments.py`
experiment numbers (`1`, `3`) to render only those. Within a sweep every file gets the **same**
gain, all three notes included, so loudness differences are real. `stats.py file.wav ...` prints
the figures for any WAV.

One-off renders: `./build/cloudlab out.wav key=value ...` (`--help` lists the keys).
`mat=bar` (or any ModalMaterials name) renders a table instead of a cloud, for reference.

## The cloud, and what was decided

| Key | Default | Meaning |
|---|---|---|
| `count` | 16 | modes, 1..64 (`ModalBank::kMaxModes`) |
| `mean` | 1 | centre of the cloud, octaves above the fundamental |
| `sigma` | 0.5 | spread, octaves |
| `seed` | 1 | the cloud is the same for a given seed, on every note and every machine |
| `keep1` | 1 | 1: mode 0 is the fundamental, ratio 1, and the other count−1 are drawn. 0: all are drawn |

- **log2(ratio) ~ Normal(mean, sigma).** In octaves, so the cloud is symmetric in pitch and
  `sigma` reads as a musical width, like the damp bell's.
- **The fundamental is kept by default** (`keep1=1`). Without it nothing is sure to sit at the
  note's pitch, and a cloud centred two octaves up would play the wrong note. `keep1=0` is there to
  hear what the fundamental contributes.
- **Amplitude is 1/sqrt(ratio), damp is 1**, as for every ModalGenerators series. The bank's
  controls do the shaping: Damp Tilt, Brightness, Strike Position. The kept fundamental gets the
  same law, amplitude 1.
- **Truncated at ratios 1/16 to 120.** A draw outside is redrawn. 120 is ModalGenerators'
  `kMaxRatio`. 1/16 keeps a low cloud from spending modes under about 4 Hz at C2.
- **Sorted by ratio**, because the bank pans by mode index and every table and series rises.
- **Repeatable across machines.** The Gaussian is Box-Muller on `std::mt19937`, whose output the
  C++ standard fixes. `std::normal_distribution` is implementation-defined, so it would draw a
  different cloud on the Mac (libc++) than on Linux (libstdc++). A plugin generator must keep this,
  or a preset would sound different on each platform.
- **Draws are shared within a seed.** Every cloud with seed 1 uses the same sequence of normal
  draws, so in each sweep only the named thing changes. The sigma sweep stretches one shape and
  the mean sweep transposes it. A smaller count is the first draws of a larger one. With
  `keep1=0`, the fundamental is replaced by one more draw (seed 1: ratio 1.037 instead of 1).

## Experiments

Each one is at C2, C4 and C6, with 16 modes centred one octave up, sigma 0.5, seed 1, the
fundamental kept, unless the sweep changes it.

1. **Sigma**: 0.02, 0.1, 0.25, 0.5, 1, 2 octaves, with the Bar material for reference.
2. **Mean**: −1, 0, 1, 2, 3 octaves above f0.
3. **Count**: 4, 8, 16, 32, 64.
4. **Seed**: 1, 2, 3, each with the fundamental kept and not.
5. **Unison**: a tight cloud on the fundamental (mean 0), sigma 0.005 to 0.1 octave (6 to 120
   cents). That makes a chorus or a detuned string, which `expandUnison` already does another way.

## Found from the measurements (nothing has been listened to yet)

These come from `stats.txt` and the spectrograms. **Nobody has heard the renders**, so nothing here
says whether a cloud sounds good, fuses into a pitch, or is worth a generator.

- **A cloud's attack is 6–8 dB hotter than a table's at the same energy.** The bank
  normalises energy, not peak, and every mode starts in phase at the strike. With 16 near-equal
  amplitudes the peaks add. In sweep 1, Bar peaks at 0.31–0.37 and the sigma 0.5 cloud at 0.78,
  with nearly the same RMS (0.021 against 0.023 at C4). The cloud's first-100 ms crest factor is
  14 dB against Bar's 8 dB. A generator would make the Level jump when the material is switched,
  unless it is trimmed.
- **Peak grows as sqrt(count), and RMS barely moves.** In sweep 3, from 4 to 64 modes the peak
  goes 0.22, 0.32, 0.43, 0.63, 0.88 (×1.41 per doubling), while RMS at C4 stays at 0.013–0.015.
  So the crest factor goes from 8.5 to 18.7 dB: more modes make more of a click on top of the same
  body.
- **Damp Tilt lengthens modes below the fundamental.** T60 scales as ratio^−tilt, so a mode at
  ratio 0.3 rings 3× as long as the fundamental (6 s at the default 2 s). A cloud with weight below
  f0 rings much longer than "Decay" says. The mean −1 cloud measures T60 5.1–5.6 s, and the
  sigma 2 cloud 5.3 s, against 1.7 s for the base cloud. Every table and series starts at
  ratio 1, so this case never came up before.
- **Beat rates are not note-relative.** Sigma is in octaves, so the spacing in Hz, and so every
  beat rate, scales with the note. C6 beats 16× as fast as C2. In the base cloud 12 of the 15
  neighbouring pairs are under 30 Hz apart at C2, 7 at C4 and 5 at C6. The unison sweep shows it
  most: at sigma 0.005 the RMS is 0.094 at C2 and 0.048 at C6, because C2's modes stay in phase for
  about a second and C6's drift apart in under a tenth. The same cloud may
  be a slow shimmer low and a rough buzz high.
- **The measured T60 wobbles with the beating.** In sweep 4 the same cloud measures T60 1.5–2.2 s
  depending on note and seed, while every mode's own T60 is fixed. The Schroeder curve follows
  the beats, so the T60 column is a rough guide for clouds.
- **The spectral centroid tracks the note** (C4 to C6 is ×3.7–4.1 in every sweep) and the mean
  (sweep 2 rises from mean −1 to mean 2). But at mean 3 it **falls**: 265 Hz at C4, against 373 Hz
  at mean 2. The cloud sits around ratios 5–17, and Damp Tilt gives those modes T60s of 0.16–0.47 s,
  so what lasts is the fundamental. Peak also drops to 0.28, because the high modes' 1/sqrt(ratio)
  amplitudes give little to the sum.
- **Keeping the fundamental changes little at mean 1, sigma 0.5.** In sweep 4 each `_free` render
  is within 1 dB RMS and 0.15 s T60 of its kept twin. The fundamental is one mode of 16, with
  amplitude 1 against the others' 0.5–0.9. The biggest gap (seed 2, 1 dB at C4) is where the draw
  put a mode 2 cents from the kept fundamental, and the pair beat slowly. Whether the fundamental
  fixes the perceived pitch is a listening question.
- **Wide clouds lose modes at the edges.** At sigma 2, C6's top mode (ratio 39.6, 41 kHz) is past
  Nyquist and muted, and C2's bottom (ratio 0.30, 19 Hz) is under 20 Hz. At sigma 1 or less, all 16
  are audible on all three notes.
- **Seeds differ less than sigma or mean do.** Across seeds 1–3, RMS is within 1 dB and the
  centroid within 25%, but the lowest mode moves (seed 3 draws ratio 0.83, under the fundamental).
  Which seed sounds best is for the ear. Presets would want the seed as a stored setting, not a
  fresh draw per note.

## What to listen for

- Sweep 1: from what sigma does the cloud stop sounding like a chorused octave (0.02) and start
  sounding like a struck object? Does sigma 2 still have a pitch?
- Sweep 2: does mean 2 or 3 still sound like the note played, or like a higher one?
- Sweep 3: are 64 modes a smoother texture than 16, or just a louder click?
- Sweep 4: do the free clouds (no fundamental) lose the pitch?
- Sweep 5 against StruckSynth's unison: is a Gaussian detune any different from evenly spaced
  copies?
- All sweeps, C2 against C6: is the beating useful at both ends, or does sigma want to narrow as
  the note rises?

## Not modelled

- **No per-mode damping or amplitude randomness.** Only the frequencies are random. A real
  "cloud" object (a cymbal, a gong) has random amplitudes and decays too.
- **No interaction with Stretch.** It applies (`stretch`), but the sweeps leave it at 0.
- **One strike, no velocity sweep.** The bank is linear, so velocity is only level here.
