<div style="display: flex; align-items: center; gap: 0.75rem; margin: 1.5rem 0 0.5rem;">
  <a href="https://www.unlikelyware.com" target="_blank" rel="noopener noreferrer">
    <img src="Unlikelyware.png" alt="Unlikelyware" style="width: 56px; height: auto; border-radius: 10px; margin: 0; display: block;">
  </a>
  <h1 style="margin: 0;">StruckGaussian</h1>
</div>

<p>
  <em>StruckGaussian is a CLAP instrument for macOS and Linux. Fine adjustment uses the Control key on both: on the Mac that is Control, not Cmd.</em>
</p>

::: {.callout .callout-note .card}
## On This Page
[Overview](#overview) · [Getting Started](#getting-started) · [The Panel](#panel)

**The controls —** [The Spectrum](#spectrum) · [Strike](#strike) · [Ring](#ring) · [Object](#object) · [Decay](#decay) · [Global](#global) · [Timbre](#timbre) · [Expression](#expression)

**Playing —** [Voices and MIDI](#voices-midi) · [Presets](#presets) · [Tips](#tips) · [Troubleshooting](#troubleshooting)
:::

::: {.callout .callout-note .card}
## Overview {#overview}
StruckGaussian is a **modal-synthesis instrument**: it sounds like a struck object rather than an oscillator through a filter. What's unusual is the object. Its modes, the frequencies it rings at, aren't a bar's or a bell's: they are a **cloud**, scattered at random on a bell curve around a pitch you choose. A narrow cloud is a chorus on one pitch; a wider one a struck object nobody has built; a wide one a clang.

Each note goes through three stages:
1. **Strike** makes a short excitation burst: a smooth mallet bump or a burst of noise, from a single sample long (the hardest, brightest hit) to about 20 ms or 1.6 periods of the note (the softest).
2. **ModalBank** is a bank of up to 64 tuned resonators, one per **mode**. The burst sets them ringing. Where they sit comes from the **cloud** (Centre, Width, Modes, Seed); how loud each starts from **Focus**; how long each rings from **Decay** and **Damp**.
3. **Final** sends the bank's stereo output to the host.

**There is no amplitude envelope.** The resonators' own decay is the envelope, so a note rings on after you let go of the key, for as long as the decay says, and the voice frees itself once it has rung down to silence. Note-off doesn't stop a ringing note; it only ends your control of it.
:::

::: {.callout .callout-note .card}
## Getting Started {#getting-started}
1. **Load StruckGaussian** on an instrument track in your host (REAPER, Bitwig, StageHand, or any CLAP host).
2. **Play.** The default patch (the same as the **Init** preset) is a 16-mode cloud half an octave wide, centred on the note, with a 2-second decay.
3. **Pick a preset** from the **Preset** menu at the top right; see [Presets](#presets).
4. **Change the cloud** with **Width** and **Centre** in the Object panel, and watch the spectrum at the top left redraw. **Seed** picks a different cloud of the same shape.
5. **Make the note's pitch stand out** with **Focus** in the Timbre panel: at 1 the modes nearest the note are loudest. Narrow **Focus Wid** to make the others fall away faster.
6. **Change how long it rings** with **Decay**, and how quickly its upper modes die away with **Damp**.
7. **Change the mallet** with **Hardness** in the Strike panel: lower is harder and brighter.

The host saves every parameter with its project.
:::

::: {.callout .callout-note .card}
## The Panel {#panel}
**The header:** the Unlikelyware logo (a click opens unlikelyware.com), the title, **Save** and **Load** (see [Presets](#presets)), **Help**, and the **Preset** menu.

**Below it, three rows:**
- **Top:** the [spectrum](#spectrum), and the **Strike** and **Ring** panels, whose controls are laid out two rows deep.
- **Middle:** **Object** and **Decay**.
- **Bottom:** **Global**, **Timbre** and **Expression**. Global's two menus, Voices and MPE, share its first column.

Each panel has its own colour: steel for Global, amber for Strike, pink for Ring, green for Object, violet for Decay, copper for Timbre and teal for Expression. Each knob shows its name above it and its value below.

**Resizing:** drag the host window's corner. The panel keeps its shape (1080 × 780) and scales everything in it; the smallest size is 864 × 624.
:::

<details class="disclosure">
<summary class="disclosure-title subtle">Knobs and menus</summary>

::: disclosure-content
| Gesture | Result |
|---|---|
| Drag up or down | Change the value. About 200 points of drag covers the whole range. |
| Control-drag | Ten times finer (Control on the Mac too) |
| Scroll wheel | One step is 5% of the range; with Control, 0.5% |
| Shift-click | Back to the default value |
| Click a menu | Opens its list; click a choice, or click elsewhere to close it |

Knobs with a time (Glide, Decay) work on a logarithmic scale, so the short end gets as much travel as the long end. Times under a second read in ms, longer ones in s.
:::

</details>

::: {.callout .callout-note .card}
## The Spectrum {#spectrum}
The display at the top left draws the current cloud's modes as bars:
- **Across:** each mode's frequency, on a logarithmic scale, so octaves are equal widths. The axis is centred on the note's pitch, marked by a thin line labelled **f0**, so modes below the note and above it are easy to tell apart, and the line stays put as Width changes.
- **Height:** how loud each mode starts, relative to the loudest. Focus is what shapes this.
- **Opacity:** how long each mode rings. A more solid bar outlasts a fainter one.
- **Thin pink bars:** with **Ring** or **Ring Fb** up, the new frequencies the [ring mod](#ring) adds. They're worked out exactly, feedback included. Where several land on the same spot, the loudest is drawn. Ones that would go past half the sample rate are drawn where they fold back down, as they do in the sound.

It's drawn for a nominal C3 (moved by **Pitch**), for the **Seed**'s cloud: with **Reseed** on, every note you play draws a different one. It holds no controls.
:::

::: {.callout .callout-note .card}
## Strike {#strike}
The exciter: what hits the object, and how hard.
- **Strike** (Mallet / Noise): **Mallet** is a smooth bump, like a mallet's contact; **Noise** is a burst of white noise of the same length, for a scraped or brushed attack. Default Mallet.
- **Hardness** (0–1, default 0.30): how long the contact lasts. **0** is a single-sample impulse, the hardest and brightest strike, which sets every mode ringing equally. Higher values lengthen the contact, which rolls off the upper modes, as a softer mallet does. Hardness changes the tone, not the level: the burst is scaled so a softer hit isn't louder.
- **Vel>Level** (0–1, default 1.00): how much velocity sets loudness. At **1** loudness follows velocity fully; at **0** every note strikes at full level.
- **Vel>Hard** (0–1, default 0.50): how much velocity hardens the strike. A harder hit shortens the contact, by up to Hardness × Vel>Hard at full velocity, so harder playing is brighter as well as louder.
- **Contact** (Note-Rel / Wall-Clk): how the contact length is measured.
  - **Note-Rel** (default): in periods of the played note. At Hardness 1 the contact spans 1.6 periods, so the strike sits at the same place relative to the fundamental on every key, and level and tone stay even across the keyboard. (Capped at 100 ms, which matters only below about 25 Hz.)
  - **Wall-Clk**: in fixed time, up to 20 ms at Hardness 1, like a real mallet: low notes come out fuller and high notes brighter and more percussive.
:::

::: {.callout .callout-note .card}
## Ring {#ring}
An experimental ring modulator on each voice. It multiplies the sound by a sine that follows the note, and **adds** the result to the struck sound, which stays at full level. With both levels at 0 (the default) it does nothing.
- **Ring** (0–1, default 0): how much of the struck sound, times the sine, is added. Each mode gains two new frequencies: the mode plus the sine's frequency, and the mode minus it (folded back through 0 Hz when the sine is higher). At **1** each is half the mode's level.
- **Ring Fb** (0–0.98, default 0): feeds the output back into the ring modulator, one sample later. Each trip round adds the sine's frequency again, so the new frequencies go on in steps: the mode ± 1, 2, 3… times the sine's frequency. Higher Ring Fb makes the series longer and buzzier and raises the level, more on low modes than high, so watch **Level**. It stops short of 1, where the loop would run away. It works with Ring at 0 too.
- **Ring Pitch** (−4 to +4 oct, default 0): the sine's pitch, in octaves from the note. The sine follows the played note and pitch bend, but not the Object panel's **Pitch**. At **0** the sine is the note itself, so the note's own mode (with **Keep Fund** on) gains its octave, plus a DC offset you can't hear. Whole octaves (**±1**) keep the new frequencies tied to the note; other settings add tones unrelated to it. Since a cloud's modes aren't harmonics of the note, their new frequencies are inharmonic whatever the setting.

The fed-back signal is limited to ±1, so very loud notes with Ring Fb near the top distort. The spectrum doesn't show that distortion.
:::

::: {.callout .callout-note .card}
## Object {#object}
What is struck: its modes and its pitch. The modes' pitches are drawn at random, in octaves from the note, from a bell curve (a Gaussian) centred on **Centre** and as wide as **Width**.
- **Pitch** (−4 to +4 oct, default 0): transposes the instrument, in octaves. It's continuous, so 1/12 of an octave is a semitone.
- **Centre** (−2 to +4 oct, default 0): where the cloud sits, in octaves above the played note. **0** centres it on the note; **1** an octave above it.
- **Width** (0.002–3 oct, default 0.50): how far the modes spread either side of Centre. Below about **0.02** they bunch onto one pitch and beat against each other, a chorus; around **0.5** the cloud is a struck object; above **1** it's a wide clang. Changing Width spreads the same cloud wider or narrower without moving it.
- **Modes** (1–64, default 16): how many modes the cloud has. More is denser and smoother; fewer is simpler and costs less CPU. A mode that would land below 1/16 or above 120 times the note is left out, so a very wide cloud can have fewer than this. Modes above the Nyquist frequency are muted.
- **Seed** (1–100, default 1): which cloud. Two seeds give two different clouds of the same shape, like two bells from one mould. The same seed is the same cloud every time, on every computer.
- **Reseed** (Off / Per Note, default Off): **Per Note** draws a new cloud at every note-on, from the same Centre, Width and Modes, so no two hits are alike, and the notes of a chord differ too. **Off** plays the Seed's cloud on every note.
- **Keep Fund** (No / Yes, default Yes): **Yes** makes one mode the note's own pitch, so the cloud always holds the note. **No** draws every mode, so the pitch you hear is wherever the cloud is loudest, and moves a little as Width changes.
- **Stretch** (0–19, default 9): inharmonicity. It steps through a table of values: **9 is none**, higher steps push the upper modes sharp, lower steps pull them flat. The steps are −0.5, −0.2, −0.1, −0.05, −0.02, −0.01, −0.005, −0.002, −0.001, **0**, 0.001, 0.002, 0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0, and each mode's frequency ratio is raised to the power 1 + that value. The knob shows the step number, not the value.
:::

<details class="disclosure">
<summary class="disclosure-title subtle">How the cloud is drawn</summary>

::: disclosure-content
- Each mode's distance from the note, in octaves, is a random draw from a Gaussian: on average 0, spread by Width. The draws are then shifted so they average exactly 0, so the cloud's centre is exactly **Centre** for every seed: Width spreads the modes evenly either side without moving them.
- With **Keep Fund** on, the note's own pitch is added as one more mode, and the rest are drawn.
- The random numbers come from a generator whose output is fixed by the C++ standard, so a seed draws the same cloud on macOS and Linux.
- A cloud's modes start in step at the strike, so their peaks add up: without a correction, more modes or near-equal levels would hit several dB harder for the same overall energy. StruckGaussian scales each strike to make up for it, so **Modes**, **Width** and **Focus** change the tone of the hit, not its level.
:::

</details>

::: {.callout .callout-note .card}
## Decay {#decay}
How long the object rings, and which modes die first.
- **Decay** (20 ms to 60 s, default 2.00 s): the T60 of a mode at the note's own pitch, the time it takes to fall by 60 dB.
- **Damp** (0–3, default 0.90): how much faster higher modes die. Each mode's decay is Decay × ratio^−Damp, where ratio is its frequency over the note's. At **0** every mode rings as long, like metal; around **2** the upper modes die quickly, like wood.

**Damp works below the note too:** a mode under the note's pitch rings *longer* than Decay. A cloud centred below the note, or a wide one, rings longer than Decay says, and keeps voices alive longer.
:::

::: {.callout .callout-note .card}
## Global {#global}
- **Voices** (Mono / Poly 10 / Poly 20 / Poly 32, default Poly 10): how many notes can sound at once. See [Voices and MIDI](#voices-midi).
- **Glide** (0.1 ms to 2 s, default 10 ms): the time to slide from one note's pitch to the next. **Only in Mono**; in the Poly modes each note starts on its own pitch.
- **Level** (0–2, default 0.50): the output level, as a gain.
- **Spread** (0–1, default 0.30): spreads the modes across the stereo field, lowest on the left to highest on the right. **0** puts them all in the centre.
- **MPE** (Off / On, default Off): treats the input as an MPE controller (a lower zone). See [Voices and MIDI](#voices-midi). Presets don't change it: it describes your controller, not the sound.
:::

::: {.callout .callout-note .card}
## Timbre {#timbre}
How loud each mode starts. Focus is read **at the strike**, so turning it while a note rings changes nothing until the next note.
- **Focus** (0–1, default 0): **0** leaves the modes at about equal levels. **1** shapes them into a second bell curve, centred on **Focus Ctr**, so the modes nearest it are loudest and the others fall away. In between, it's a blend.
- **Focus Ctr** (−2 to +4 oct, default 0): where the loudness peaks, in octaves above the note. **0** makes the note's own pitch loudest.
- **Focus Wid** (0.05–3 oct, default 0.50): how fast loudness falls away from the peak. Narrow it for steep sides: at **0.1**, modes a few tenths of an octave out are 20 to 40 dB down; at **0.05** only the modes right at the peak are left. Wider than the cloud, it barely changes anything.

The overall level stays about the same as Focus changes, so it's a tone control, not a volume control.
:::

::: {.callout .callout-note .card}
## Expression {#expression}
What a controller's **pressure** and **timbre** do to a ringing note. Each knob is 0 by default, which does nothing, so a controller that sends no pressure or timbre sounds the same with these up.
- **Press>Decay** (−4 to +4 oct, default 0): pressure changes the decay time of the ringing note, by this many octaves at full pressure (+2 rings 4× longer). Positive: press to keep it ringing, ease off to damp it. Negative: pressing damps it, like a hand on a bar. It works all through the note.
- **Tmb>Tilt** (−3 to +3, default 0): timbre adds to **Damp**, by this much at full timbre. Positive darkens the tail, negative brightens it. It works all through the note.

**Pressure** is channel pressure, poly pressure, or a CLAP Pressure expression. **Timbre** is CC 74 on the note's channel, MIDI 2.0 per-note controller 74, or a CLAP Brightness expression. A per-note value wins over the channel's. Changes are smoothed over 2 ms.
:::

::: {.callout .callout-note .card}
## Voices and MIDI {#voices-midi}
**Voices.** In the Poly modes each note gets its own voice, up to 10, 20 or 32; past that, a sounding voice is taken for the new note. Because notes ring after note-off, a fast passage with a long Decay can use many voices: choose Poly 20 or 32 for that. **Mono** plays one voice: a new note while another is held strikes the same resonators again and glides them to the new pitch (see **Glide**), so the old note's ring carries into the new one. Letting go of the new note goes back to the note still held.

**Sustain pedal** (CC 64) holds released notes in the Poly modes. Since a struck note rings on anyway, the pedal mostly matters for what still controls the note: see *Released notes* below.

**MIDI 2.0 first.** StruckGaussian prefers MIDI 2.0 from the host, so a note's exact pitch (MIDI 2.0 Pitch 7.9) reaches the resonators unchanged. MIDI 1.0 works too; its values are scaled to MIDI 2.0's, so velocity 127, CC 127 and a full pitch bend are full scale. In MIDI 2.0, a note-on with velocity 0 is a note-on; **Vel>Level** decides how loud it is.

**A note is note number + channel + group.** The same key on two channels is two independent notes.
:::

<details class="disclosure">
<summary class="disclosure-title subtle">Pitch bend</summary>

::: disclosure-content
- **Channel pitch bend** (MIDI 1.0 or 2.0) is **±2 semitones** by default. Each note follows the bend of the channel it was played on, so with one note per channel it's per-note bend.
- **RPN 0** (pitch bend sensitivity) sets a channel's range.
- **MIDI 2.0 per-note pitch bend** bends only the note it names, over the same range as its channel's bend. A controller can give per-note bend a range of its own with MIDI 2.0 registered controller 7, as KeyboardEDO does.
- **Pitch 7.25** (MIDI 2.0 per-note controller 3) sets an absolute pitch for a key, and the bends offset from it. It retunes the sounding note and stays with the key: later notes on that key start at it, unless the note-on carries its own Pitch 7.9 pitch. Per-Note Management Reset clears it.
- Bends come after **Glide**, so glide never smears a bend, and they bend a ringing tail smoothly rather than re-striking it. Bend changes are smoothed over 2 ms, and a note starts at its bent pitch.
- Per-note bend resets on each note-on. One sent before its note-on is ignored.
:::

</details>

<details class="disclosure">
<summary class="disclosure-title subtle">MPE</summary>

::: disclosure-content
With **MPE** on, StruckGaussian treats its input as an MPE **lower zone**: channel 1 is the master channel, 2–16 are the member channels, one per note. StruckGaussian only receives MPE; it never sends it.
- A member channel's bend range is **±48 semitones** by default, and the master channel's bend (±2) is added to every note, so the controller's pitch wheel moves everything on top of each note's own slide.
- RPN 0 on any member channel sets the range of all of them.
- Pressure and timbre (CC 74) drive the [Expression](#expression) knobs. A member note takes the higher of its own value and the master channel's.
- Ranges set by RPN 0 go back to their defaults when MPE is switched on or off, and on an MPE Configuration Message.
:::

</details>

<details class="disclosure">
<summary class="disclosure-title subtle">Released notes</summary>

::: disclosure-content
A struck note rings for seconds after its note-off. Once it's released (or held only by the sustain pedal), its own per-note messages no longer reach it, as the MPE specification says: so when an MPE controller re-centres a member channel for the next note, the ringing tail stays where it was. The master channel's bend still moves it.

With MPE off, an ordinary pitch wheel still bends released notes. MIDI 2.0 per-note messages stop at note-off. CLAP note expressions sent by note id keep working until the note has rung out.
:::

</details>

<details class="disclosure">
<summary class="disclosure-title subtle">CLAP note expressions and note end</summary>

::: disclosure-content
Hosts such as Bitwig send per-note expression as CLAP note expressions:
- **Tuning** becomes an exact per-note pitch (as Pitch 7.25), so any amount of tuning is exact rather than limited to the bend range. It lasts for its note only.
- **Pressure** and **Brightness** drive the [Expression](#expression) knobs' pressure and timbre.
- Volume, Pan, Vibrato and Expression (the CLAP expression of that name) are ignored.

For a note the host sends as a CLAP note, StruckGaussian reports the note's end when it actually stops sounding: when it has rung down to silence, when its voice is taken, or (in Mono) when it leaves the held notes. So the host keeps that note's modulation running through its tail.
:::

</details>

<details class="disclosure">
<summary class="disclosure-title subtle">All Notes Off and All Sound Off</summary>

::: disclosure-content
- **All Notes Off (CC 123)** releases every note on the channel it arrives on, pedal-held ones included. They ring out like any released note.
- **All Sound Off (CC 120)** silences that channel's notes at once, ringing tails included.
:::

</details>

::: {.callout .callout-note .card}
## Presets {#presets}
The **Preset** menu at the top right holds the factory presets, in alphabetical order:

| Preset | Cloud | Decay | Character |
|---|---|---|---|
| Chorus Unison | 12 modes, Width 0.008 | 3.5 s | Every mode on the note: a chorused single pitch |
| Clang Cluster | 20 modes, 1.5 oct up | 2.2 s | Hard strike, bright and inharmonic |
| Dark Gong | 40 modes, half an octave under | 3 s | Soft mallet; low modes ring long |
| Focused Glass | 32 modes, Width 0.6 | 3 s | Focus 1 on the note, narrow: a clear pitch in a halo |
| Halo | 48 modes, Width 1 | 4 s | A wide cloud with a broad loudness peak on the note |
| Init | 16 modes, Width 0.5 | 2 s | The default patch |
| Never Twice | 24 modes, 0.8 oct up | 2.5 s | Reseed Per Note: every hit is a new cloud |
| Octave Shimmer | 16 modes, an octave up, Width 0.015 | 3 s | The note with a chorused octave above it |
| Wood Cluster | 10 modes, Width 0.35 | 0.5 s | Heavy damping: a short, woody knock |

- **Choosing a preset sets every sound parameter**, Voices, the Expression knobs and Ring included (every factory preset has Ring off). It leaves the **MPE** switch alone.
- The menu shows **--** until you choose one, and again when the editor is reopened. Changing a knob after choosing a preset doesn't change the name shown.

**Your own patches:** **Save** and **Load**, left of Help, write the patch to a file and read one back, through the system's file dialog. The first time, the dialog opens on **Documents/StruckGaussian**; after that, on the last folder you used.
- A saved patch has every sound parameter, like a factory preset, and leaves out **MPE**. Loading one sets those parameters and leaves MPE alone.
- The menu then shows the patch's name, dimmed, in place of a factory preset's.
- The file is the same JSON as the factory presets, so a patch you save can be put in the source's `Presets` folder to build it in as a factory preset.
- The host still saves the patch with its project, whether or not you saved it to a file.
:::

::: {.callout .callout-note .card}
## Tips {#tips}
- **Width is the main knob.** Narrow for a pitch with chorus, middle for an object, wide for a clang. Then **Seed** through a few clouds of that width: they differ more than you'd expect.
- **For a clear pitch from a wide cloud**, keep **Keep Fund** on and set **Focus** to 1 with **Focus Wid** around 0.1–0.2.
- **For a bell-like cloud**, put Centre around 1 (an octave up) with Keep Fund on: the note plus a cloud of overtones.
- **Damp is the material knob.** Low Damp sounds like metal or glass, high Damp like wood or skin.
- **Low and wide clouds ring long**, because Damp lengthens modes below the note. Shorten **Decay** to keep voices from piling up.
- **Beating speeds up with pitch.** Modes a fixed fraction of an octave apart beat slowly on low notes and fast on high ones, so a narrow cloud shimmers in the bass and buzzes at the top.
- **Reseed Per Note** suits repeated notes and drum-like parts, where a fixed cloud would sound mechanical.
- **Fewer Modes** makes a simpler sound, and costs less CPU.
- **For an even keyboard**, keep **Contact** on Note-Rel. Wall-Clk makes the bass boomier and the top pingier, as real mallets do.
- **For a hand-damped object on a pressure controller**, set Press>Decay negative: press into a note to stop it.
- **Ring Pitch in whole octaves** (0, ±1) ties the ring mod to the note; anything else pulls the sound towards a clang. Add **Ring Fb** slowly: the top of its range is much louder.
:::

::: {.callout .callout-note .card}
## Troubleshooting {#troubleshooting}
- **Notes keep ringing after I let go.** That's the design: a struck object rings until it dies away. Shorten **Decay**, raise **Damp**, or use a negative **Press>Decay** to damp it by pressure.
- **Notes ring much longer than Decay.** The cloud has modes below the note, which Damp lets ring longer. Raise **Centre**, narrow **Width**, or shorten **Decay**.
- **Notes cut off in fast passages.** They're running out of voices. Choose Poly 20 or Poly 32, or shorten Decay.
- **Glide does nothing.** It only works in **Mono**.
- **The pitch wanders as I change Width.** **Keep Fund** is off, so the pitch you hear is the loudest mode, which moves. Turn Keep Fund on.
- **Focus barely does anything.** **Focus Wid** is wider than the cloud. Narrow it, or widen the cloud.
- **Raising Modes changes little at a wide Width.** Some draws land outside 1/16–120 times the note and are left out.
- **Focus doesn't change a held note.** It's read at the strike; play the next note.
- **The Expression knobs do nothing.** The controller isn't sending pressure or CC 74, or the knobs are at 0.
- **Bends are huge.** MPE is on and the controller isn't an MPE controller: member channels bend ±48 semitones. Turn **MPE** off.
- **Distortion.** Lower **Level**. Many loud notes ringing together add up. With **Ring Fb** near the top, the feedback limit distorts loud notes too; lower Ring Fb or Level.
- **A rebuilt StruckGaussian doesn't show its changes.** Quit and restart the host: a running host keeps the plugin it loaded first.
:::
