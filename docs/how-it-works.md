# How StruckGaussian works

The detail behind [StruckGaussian](../README.md): the voice, its modules and how it takes MIDI 2.0.
The [help page](../Resources/help.md) describes the controls.

## Overview

A cross-platform **CLAP instrument** from Unlikelyware: a struck object whose modes are a
**Gaussian cloud** around the note. It is StruckSynth (a shaped excitation burst into a bank of
tuned resonant modes) with the material tables replaced by a random draw: the modes' pitches are
scattered on a bell curve around a centre you set, from a tight chorus on one pitch to a wide,
clangy cluster. A seed picks which cloud, so a preset always plays the same object, or every note
can draw its own.

## The mode cloud

Each mode's pitch, in octaves from the note, is drawn from a Gaussian: log2(ratio) ~ Normal(Centre,
Width). The draws are recentred so they average exactly Centre, so Width spreads the modes evenly
either side without moving the cloud. A second, optional Gaussian shapes how loud each mode starts
(Focus).

| Group | Parameters | Notes |
|---|---|---|
| Object | Centre | where the cloud sits, octaves above the played note: 0 centres it on the note |
| | Width | how far the modes spread (the Gaussian's sigma), 0.002 to 3 octaves. Below about 0.02 the cloud is a chorus on one pitch; around 0.5 a struck object; above 1 a wide clang |
| | Modes | how many, 1 to 64. A draw that lands outside 1/16 to 120 times the note is dropped, so a very wide cloud can have fewer |
| | Seed | which cloud, 1 to 100: one seed is one object, the same on every platform |
| | Reseed | Per Note: every note-on draws a new cloud, so no two hits are alike (chords too) |
| | Keep Fund | Yes: one mode is always the note's own pitch, so the cloud can't lose the note |
| | Stretch | inharmonicity, as in StruckSynth |
| Timbre | Focus | how loud each mode starts. 0: about equal; 1: a Gaussian peaked at Focus Ctr, so the modes nearest it are loudest |
| | Focus Ctr, Focus Wid | the loudness peak, octaves above the note (0 = the note), and how fast loudness falls away from it. Narrow Focus Wid for steep sides: at 0.1, modes a few tenths of an octave out are 20 to 40 dB down |
| Decay | Decay, Damp | T60 of the note's own pitch, and how much faster higher modes die. Damp also makes modes **below** the note ring longer, so a cloud centred low rings long |

The strike is scaled so the hit stays at about the same level whatever Modes, Width and Focus are:
the bank keeps energy, not peak, constant, and a cloud of near-equal modes otherwise hits several dB
harder as modes are added. The spectrum display draws each mode's starting level, its decay as
opacity, and a line at the note's pitch (f0). Strike Position and Brightness, which StruckSynth has,
are gone: both moved the loudness peak, which is Focus's job, and Strike Position's law assumes a
string's mode shapes, which a cloud doesn't have. `tools/cloudlab/` is the offline renderer the cloud
was prototyped in, with what its measurements showed.

## The voice

**Polyphonic struck-resonator synth with a full custom GUI.** The PM framework drives a
`VoiceManager` (polyphony, note-stealing, mono/legato) rendering the voice
**Strike → ModalBank → Final**, with pitch tracking via **PitchConverter → Slew** (glide) into
the bank's pitch input. There is no VCA envelope: the bank's own T60 decay is the amplitude
envelope, and a voice frees itself once it rings down to silence, so the tail survives
note-off.

- **Strike** (exciter): Mallet or Noise burst, hardness, velocity → level / hardness, contact
  mode. MIDI 1.0 values are scaled to MIDI 2.0 with the spec's Min-Center-Max method, so velocity
  127 is full scale (as are CC 127 and a full pitch bend), and 64 / 8192 the exact centre. A MIDI 2.0
  note-on with velocity 0 is a note-on, as the spec says; Vel>Level decides how loud it is.
- **ModalBank** (resonator): up to 64 modes from the Gaussian cloud (above), stretch, decay (T60),
  damping tilt, loudness Focus, stereo spread.
- **Ring** (experimental): a ring modulator on each voice, a sine at the note (Ring Pitch octaves
  from it) whose product is **added** to the struck sound: `y = x + sin · (Ring · x + Ring Fb ·
  clip(y[n−1]))`. Ring alone gives each mode one pair of sidebands; Ring Fb feeds the output back
  through a one-sample loop, so the sidebands run on in steps of the sine's frequency. Ring Fb stops
  at 0.98, short of the loop's pole. The spectrum draws the sidebands, solved exactly (`src/Module/RingMod.hpp`).
- **Voices**: Poly 10, Poly 20, Poly 32, or Mono (legato, with glide).
- **Presets**: eight cloud presets and Init in `Presets/*.json`, baked into the plugin at build
  time.

MIDI notes are sample-accurate: `process()` splits rendering at each event's frame offset.
Parameter changes are applied per host block, and the note port advertises (and prefers) the MIDI
2.0 dialect, so a 7.9-pitch NoteOn reaches the resonator unchanged.

### Pitch bend: channel and per-note

Every voice has a `PitchBendConverter` (`src/Module/PitchBendConverterModule.cpp`, ported from
PlainOldSynth) feeding ModalBank's second pitch input. That input comes **after** the Slew, so glide
never smears a bend, and it is scaled on its own, so a fixed-pitch body (Pitch Mod Level 0) still
bends. It sums two bends:

- **Channel pitch bend** (MIDI 1.0 or 2.0), **±2 semitones** by default. Each voice remembers the
  latest bend of every channel and applies the one its own note was played on: ordinary pitch bend
  with everything on one channel, and per-note bend when each note has its own channel. **RPN 0**
  (pitch bend sensitivity, as MIDI 1.0 CC 101/100/6/38 or a MIDI 2.0 registered controller) sets a
  channel's range.
- **MIDI 2.0 per-note pitch bend** (UMP status `0x6`). It bends only the note it names, over the same
  range as its channel's bend (**±2** by default): M2-104 v1.0 says per-note bend "acts like Pitch
  Bend in every way" and gives it no range of its own. A controller can set a separate per-note range
  per channel with MIDI 2.0 **registered controller 7** (per-note pitch bend sensitivity, from a later
  revision of M2-104), as KeyboardEDO does; that range survives a host `reset()`, since it is
  configuration the controller sends once.

Bend changes ramp over 2 ms, and a note starts at its bent pitch. ModalBank retunes its modes once per
block without resetting them, so a bend glides the ringing tail rather than re-striking it. Per-note
bend resets on every note-on and on a per-note management Reset; one sent *before* its note-on is
dropped, because no voice owns that note yet. The default range is not a host parameter.

### MPE

The **MPE** switch (Global panel, off by default) treats input as an MPE lower zone: master channel
1, member channels 2–16. MPE is accepted as an input only; StruckGaussian never sends it.

- A member channel's bend range defaults to **±48**, and the master channel's bend (±2) is **added**
  to every member note, so the controller's pitch wheel moves the whole zone on top of each note's
  own slide.
- RPN 0 on any member channel sets every member channel's range, as MPE specifies.
- **Released notes keep their pitch.** MPE §3.3.3: "control of a note ceases once Note Off has
  occurred". A struck note rings for seconds after its note-off, so when a sender centres the member
  channel for its next note, the tail stays where it was; only the master channel's bend still moves
  it. A pedal-held note counts as released. With MPE off, an ordinary pitch wheel still bends
  releasing notes. The same goes for MIDI 2.0 per-note messages: a released note stops taking them,
  except CLAP expressions addressed by note id, which last until the note's `CLAP_EVENT_NOTE_END`.
- Ranges set by RPN 0 go back to their defaults on an MPE Configuration Message (RPN 6 on channel 1
  or 16; only its reset is honoured, the zone layout is the switch's) and when the switch is flipped,
  so a range set in one mode does not leak into the other. Otherwise, like RC 7, they survive a host
  `reset()`.
- MPE's pressure and timbre (CC74) drive the **Expression** knobs (below). A member note takes the
  higher of its own value and the master channel's.
- Presets don't store the switch: it describes the controller, not the sound.

mpe-sender (an Unlikelyware test tool, not yet public) plays scripted MPE tests for listening.

### Expression: pressure and timbre

Most of a struck note is fixed at the strike: the cloud and its Focus only shape how the strike excites
each mode. What a player can still change on a ringing note is how it decays, which is what the
**Expression** panel's two knobs do. Each is 0 by default, so nothing changes until you turn it, and a
controller that sends no pressure or CC74 sounds the same with the knobs up.

| Knob | Source | What it does | When |
|---|---|---|---|
| **Press>Decay** | pressure | decay time, ± octaves at full pressure (+2 = 4× longer). Positive: press to let it ring, ease off to damp it. Negative: pressing damps it, like a hand on a bar. | continuously, on the ringing note |
| **Tmb>Tilt** | timbre (CC74) | added to **Damp** at full timbre: positive darkens the tail, negative brightens it | continuously, on the ringing note |

Pressure is channel pressure, poly pressure, or CLAP's Pressure note expression; timbre is CC74 on the
note's channel, MIDI 2.0 per-note controller #74, or CLAP's Brightness. Per-note values win over the
channel's. Both sources are PlainOldSynth's Pressure and Timbre modules, smoothed over 2 ms.

**A note is note number + channel + group.** The same key on two channels is two voices with
independent per-note state, and note-offs the engine generates itself (sustain-pedal release,
all-notes-off, panic) are built from the voice's own note-on so they match. In mono, per-note
expression for a held-but-silent note in the stack doesn't move the sounding note.

**All Notes Off (CC 123)** releases the notes of the channel it arrives on, pedal-held ones included;
they ring out like any released note. **All Sound Off (CC 120)** silences that channel's voices at
once, ringing tails included. When the host
tags notes with CLAP note ids, the ids decide instead, so a re-struck key is a new note.

### Pitch 7.25 and CLAP note expressions

**Pitch 7.25** (MIDI 2.0 registered per-note controller #3) sets an absolute pitch for the sounding
note, and both bends offset from it. Like the bends it bypasses the glide. KeyboardEDO's glide mode
sends it. As the spec makes it, it is also persistent: a tuning table, kept per key, channel and
group, that later notes on the same key inherit, whether or not a note was sounding when it arrived.
A note-on with a Pitch 7.9 attribute names its own pitch and ignores the table; Per-Note Management
with Reset (which KeyboardEDO sends before each note-on) clears the key. The table survives a host's
reset but is not saved with the plugin state: the controller sends it.

**CLAP note expressions** (`CLAP_EVENT_NOTE_EXPRESSION`, how Bitwig sends per-note expression) are
converted to MIDI 2.0 per-note messages:

- **Tuning** becomes Pitch 7.25 at key + tuning, so it is exact at any size within Pitch 7.25's
  0–128 semitone span rather than clamped to the ±2 bend range. It lasts for its note only and
  never enters the tuning table.
- **Pressure** becomes poly pressure and **Brightness** registered per-note controller #74, which
  drive the Expression knobs' pressure and timbre.
- Volume, Pan, Vibrato and Expression (the CLAP expression of that name, not the panel) are ignored.

An expression may name only a note id: tuning then looks up that note's key.

**Note end and voice info.** A struck note rings on after note-off, so the plugin reports
`CLAP_EVENT_NOTE_END` for a CLAP note when the engine actually stops playing it: when the voice rings
down to silence, is stolen, or (mono) the note leaves the held stack. Hosts such as Bitwig keep a
note's per-voice modulation and expressions running until then. Notes that arrive as MIDI events get
no note end, since a CLAP note end answers a CLAP note-on. The voice-info extension reports the voice
count (10, or 1 in Mono) and that notes may overlap on the same key, since note ids keep them apart. In the VST3 build,
clap-wrapper turns VST3 note expressions into CLAP ones, and offers hosts Tuning and Brightness as
well as Pressure.

The [DISTRHO Plugin Framework (DPF)](https://github.com/DISTRHO/DPF) is used **only as the GUI
layer** (UIExporter + NanoVG + pugl). The CLAP entry point is hand-written in `src/plugin.cpp`,
because DPF's generated CLAP wrapper drops `CLAP_EVENT_MIDI2` and advertises only the MIDI/MPE note
dialects, which would make MIDI 2.0 7.9-pitch notes unreachable. This is the same raw-CLAP shape
PlainOldSynth uses.
