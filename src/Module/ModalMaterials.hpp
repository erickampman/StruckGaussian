//
//  ModalMaterials.hpp
//
//  Mode ratio tables for the modal bank.
//
//  PROVENANCE: every table below is either derived from the governing equation
//  (noted per-table) or explicitly flagged as approximate and pending
//  verification against Fletcher & Rossing, "The Physics of Musical
//  Instruments". Nothing here is transcribed from another implementation.
//
//  Fields per mode:
//    ratio - frequency ratio to mode 0
//    amp   - relative excitation amplitude
//    damp  - decay scale relative to mode 0 (1.0 = same T60; <1 = decays sooner)
//
//  IMPORTANT — damp is 1.0 almost everywhere ON PURPOSE.
//
//  These tables originally carried a declining damp curve, which duplicated
//  what DampTilt already does: both encoded the same monotonic "higher modes
//  decay faster" trend, so they fought each other and DampTilt had far less
//  usable range than intended. At tilt 0 the top mode still decayed several
//  times faster than the fundamental, so genuinely equal ringing — the metal
//  extreme — was unreachable through the control.
//
//  The split now is:
//    DampTilt   ALL smooth frequency dependence. ratio^(-tilt) is the law.
//    damp       per-mode ANOMALIES only — a mode that departs from the trend
//               because its shape couples unusually well or badly to the
//               radiator. Real objects have these; smooth curves are not one.
//
//  Currently the only anomaly in the file is the bell's hum tone, which really
//  does outlast the prime. Do not reintroduce declining damp curves; raise
//  DampTilt instead.
//

#ifndef ModalMaterials_hpp
#define ModalMaterials_hpp

struct ModalMode {
    float ratio;
    float amp;
    float damp;
};

struct ModalMaterial {
    const ModalMode* modes;
    int              count;
    const char*      name;
};

// ---------------------------------------------------------------------------
// Ideal string, fixed at both ends.
//
// Solutions of the 1-D wave equation with y(0)=y(L)=0 give f_n = n * f_0.
// Exactly harmonic. Amplitude of a plucked string falls as 1/n^2; this is
// softened to 1/n^1.2 so upper modes stay audible under a strike exciter.
//
// Useful mainly as a sanity check: this material MUST sound harmonic and
// pitched. If it does not, the coefficient math is wrong.
// ---------------------------------------------------------------------------
static const ModalMode kStringModes[] = {
    { 1.0f, 1.000f, 1.00f }, { 2.0f, 0.435f, 1.00f },
    { 3.0f, 0.268f, 1.00f }, { 4.0f, 0.189f, 1.00f },
    { 5.0f, 0.144f, 1.00f }, { 6.0f, 0.116f, 1.00f },
    { 7.0f, 0.096f, 1.00f }, { 8.0f, 0.082f, 1.00f },
    { 9.0f, 0.071f, 1.00f }, { 10.0f, 0.063f, 1.00f },
    { 11.0f, 0.056f, 1.00f }, { 12.0f, 0.050f, 1.00f },
    { 13.0f, 0.046f, 1.00f }, { 14.0f, 0.042f, 1.00f },
    { 15.0f, 0.039f, 1.00f }, { 16.0f, 0.036f, 1.00f },
};

// ---------------------------------------------------------------------------
// Ideal free-free bar (xylophone / glockenspiel bar, untuned).
//
// The Euler-Bernoulli beam equation with free-free boundary conditions gives
// the characteristic equation
//
//     cos(bL) * cosh(bL) = 1
//
// whose roots are bL = 4.73004, 7.85320, 10.99561, 14.13717, 17.27876,
// 20.42035, 23.56194, 26.70354, ... (for n >= 3, bL ~ (2n+1)*pi/2).
//
// Frequency is proportional to (bL)^2, so the ratios are
//
//     ratio_n = (bL_n / bL_1)^2
//
//   (7.85320/4.73004)^2 =  2.7565
//  (10.99561/4.73004)^2 =  5.4039
//  (14.13717/4.73004)^2 =  8.9330
//  (17.27876/4.73004)^2 = 13.3443
//  (20.42035/4.73004)^2 = 18.6379
//  (23.56194/4.73004)^2 = 24.8137
//  (26.70354/4.73004)^2 = 31.8719
//
// Strongly inharmonic — this is the characteristic "struck metal bar" sound.
// ---------------------------------------------------------------------------
static const ModalMode kBarModes[] = {
    { 1.0000f, 1.000f, 1.00f }, { 2.7565f, 0.640f, 1.00f },
    { 5.4039f, 0.410f, 1.00f }, { 8.9330f, 0.270f, 1.00f },
    { 13.3443f, 0.180f, 1.00f }, { 18.6379f, 0.122f, 1.00f },
    { 24.8137f, 0.084f, 1.00f }, { 31.8719f, 0.058f, 1.00f },
};

// ---------------------------------------------------------------------------
// Tuned bar (marimba / vibraphone).
//
// Undercutting the bar lowers the first two overtones onto 4:1 and 10:1
// against the fundamental. This is a deliberate luthier's intervention, not a
// property of a uniform bar, so the first three ratios are the design target
// rather than a derivation. Modes above the third are barely affected by
// undercutting and drift back toward free-free bar behaviour, so they are
// continued from kBarModes scaled onto the tuned series.
//
// Marimba (wood) wants a high DampTilt; vibraphone (aluminium) wants a low one.
// Same table, different macro settings — that is the point of separating them.
// ---------------------------------------------------------------------------
static const ModalMode kTunedBarModes[] = {
    { 1.0000f, 1.000f, 1.00f }, { 4.0000f, 0.500f, 1.00f },
    { 10.0000f, 0.250f, 1.00f }, { 19.5570f, 0.130f, 1.00f },
    { 32.2360f, 0.070f, 1.00f }, { 48.0570f, 0.038f, 1.00f },
};

// ---------------------------------------------------------------------------
// Circular membrane, fixed rim (drum head).
//
// Solutions of the 2-D wave equation in polar coordinates are Bessel
// functions; the mode frequencies are proportional to the zeros j(m,n) of
// J_m. The first sixteen zeros are
//
//   2.40483 (0,1)  3.83171 (1,1)  5.13562 (2,1)  5.52008 (0,2)
//   6.38016 (3,1)  7.01559 (1,2)  7.58834 (4,1)  8.41724 (2,2)
//   8.65373 (0,3)  8.77148 (5,1)  9.76102 (3,2)  9.93611 (6,1)
//  10.17347 (1,3) 11.06471 (4,2) 11.61984 (7,1) 11.79153 (2,3)
//
// Ratios are j(m,n) / 2.40483. Note these are for an IDEAL membrane with no
// air loading; a real drum's air column pulls the low modes closer to
// harmonic, which is why real timpani are pitched and this table is not.
// Use Stretch to compensate.
// ---------------------------------------------------------------------------
static const ModalMode kMembraneModes[] = {
    { 1.0000f, 1.000f, 1.00f }, { 1.5933f, 0.700f, 1.00f },
    { 2.1356f, 0.520f, 1.00f }, { 2.2954f, 0.480f, 1.00f },
    { 2.6531f, 0.400f, 1.00f }, { 2.9173f, 0.350f, 1.00f },
    { 3.1555f, 0.310f, 1.00f }, { 3.4999f, 0.265f, 1.00f },
    { 3.5983f, 0.255f, 1.00f }, { 3.6472f, 0.250f, 1.00f },
    { 4.0588f, 0.205f, 1.00f }, { 4.1315f, 0.199f, 1.00f },
    { 4.2302f, 0.191f, 1.00f }, { 4.6008f, 0.163f, 1.00f },
    { 4.8317f, 0.148f, 1.00f }, { 4.9031f, 0.144f, 1.00f },
};

// ---------------------------------------------------------------------------
// Open tube (both ends open).
//
// 1-D wave equation with pressure nodes at both ends: f_n = n * f_0, i.e.
// the full harmonic series. Distinguished from kStringModes only by its
// amplitude profile. Tubes lose high modes faster than strings to viscous and
// thermal wall losses — that belongs in DampTilt, not in this table.
// ---------------------------------------------------------------------------
static const ModalMode kOpenTubeModes[] = {
    { 1.0f, 1.000f, 1.00f }, { 2.0f, 0.560f, 1.00f },
    { 3.0f, 0.370f, 1.00f }, { 4.0f, 0.265f, 1.00f },
    { 5.0f, 0.200f, 1.00f }, { 6.0f, 0.156f, 1.00f },
    { 7.0f, 0.125f, 1.00f }, { 8.0f, 0.102f, 1.00f },
    { 9.0f, 0.085f, 1.00f }, { 10.0f, 0.072f, 1.00f },
    { 11.0f, 0.061f, 1.00f }, { 12.0f, 0.053f, 1.00f },
};

// ---------------------------------------------------------------------------
// Closed tube (stopped at one end).
//
// Pressure antinode at the closed end, node at the open end, giving only odd
// harmonics: f_n = (2n-1) * f_0. Sounds an octave below an open tube of the
// same length and distinctly hollow — the classic clarinet/stopped-pipe
// register.
// ---------------------------------------------------------------------------
static const ModalMode kClosedTubeModes[] = {
    { 1.0f, 1.000f, 1.00f }, { 3.0f, 0.430f, 1.00f },
    { 5.0f, 0.250f, 1.00f }, { 7.0f, 0.168f, 1.00f },
    { 9.0f, 0.123f, 1.00f }, { 11.0f, 0.095f, 1.00f },
    { 13.0f, 0.077f, 1.00f }, { 15.0f, 0.064f, 1.00f },
    { 17.0f, 0.054f, 1.00f }, { 19.0f, 0.047f, 1.00f },
};

// ---------------------------------------------------------------------------
// Cantilever / tine (clamped at one end, free at the other).
//
// The Rhodes tine and the Wurlitzer reed are both cantilevers, NOT free-free
// bars. Same Euler-Bernoulli beam equation, different boundary conditions, and
// the characteristic equation flips sign:
//
//     cos(bL) * cosh(bL) = -1
//
// with roots bL = 1.87510, 4.69409, 7.85476, 10.99554, 14.13717, 17.27876.
// Frequency goes as (bL)^2, so ratios are (bL_n / bL_1)^2:
//
//    (4.69409/1.87510)^2 =  6.2669
//    (7.85476/1.87510)^2 = 17.5475
//   (10.99554/1.87510)^2 = 34.3861
//   (14.13717/1.87510)^2 = 56.8426
//   (17.27876/1.87510)^2 = 84.9127
//
// Far sparser and far more inharmonic than the free-free bar (1, 2.76, 5.40).
// The second mode is more than two and a half octaves up, so a struck tine is
// essentially a fundamental with a brief inharmonic tick on top — which is why
// amplitudes and damping fall off hard here.
//
// IMPORTANT: this is the tine ALONE. A Rhodes is this plus a coupled tonebar
// plus a strongly nonlinear pickup, and the pickup contributes most of what
// people recognise as the sound. See doElectricPiano() in the harness.
//
// Not modelled: the tuning-spring mass at the tine tip, which lowers mode 0
// relative to the rest and stretches these ratios further. Deriving that needs
// the end-mass boundary condition; the values above are the ideal uniform case.
// ---------------------------------------------------------------------------
static const ModalMode kTineModes[] = {
    { 1.0000f, 1.000f, 1.00f },
    { 6.2669f, 0.180f, 1.00f },
    { 17.5475f, 0.055f, 1.00f },
    { 34.3861f, 0.020f, 1.00f },
    { 56.8426f, 0.008f, 1.00f },
    { 84.9127f, 0.004f, 1.00f },
};

// ---------------------------------------------------------------------------
// Bell — APPROXIMATE, NOT VERIFIED.
//
// Unlike every table above, this one is NOT derived from a governing equation.
// A bell's profile is a designed surface of revolution with no closed-form
// mode solution; real partials come from measurement or FEM, and founders tune
// them deliberately (hum an octave below prime, plus tierce, quint, nominal).
//
// The set below is a plausible minor-third bell spectrum, chosen to sound
// bell-like rather than to reproduce any particular instrument. It is here so
// there is something in the bell slot during bring-up.
//
// TODO: replace with measured data from Fletcher & Rossing before this ships.
// Do not present this as physically accurate until that happens.
// ---------------------------------------------------------------------------
static const ModalMode kBellModes[] = {
    { 0.5000f, 0.600f, 1.30f },   // hum — genuinely outlasts the prime
    { 1.0000f, 1.000f, 1.00f },   // prime
    { 1.1892f, 0.780f, 1.00f },   // tierce (minor third)
    { 1.5000f, 0.620f, 1.00f },   // quint
    { 2.0000f, 0.700f, 1.00f },   // nominal
    { 2.5000f, 0.400f, 1.00f },
    { 2.6667f, 0.340f, 1.00f },
    { 3.0000f, 0.290f, 1.00f },
    { 4.0000f, 0.190f, 1.00f },
    { 5.3333f, 0.120f, 1.00f },
    { 6.6667f, 0.080f, 1.00f },
    { 8.0000f, 0.055f, 1.00f },
};

// ===========================================================================
// BODY RESONATORS
//
// Everything above models the thing that is struck. These model the thing that
// RADIATES — a guitar body, a violin corpus, a piano soundboard. They are used
// downstream of a string bank, not in place of one.
//
// Three structural differences from the tables above, and they matter:
//
//   Fixed pitch.  A body does not transpose with the note. A guitar box
//                 resonates near 100 Hz whatever you fret. So a body bank's
//                 setFundamental() is a constant, NOT driven by Pitch
//                 Converter. This is the single most important difference.
//
//   Low Q.        Bodies ring for 50-200 ms, not seconds. They colour, they
//                 do not sustain. Use DecayT60 around 0.05-0.2.
//
//   Dense modes.  A real soundboard has hundreds of modes that merge into a
//                 continuum above a few hundred Hz. These tables capture only
//                 the low, individually audible resonances.
//
// PROVENANCE — READ THIS. Unlike the bar and membrane tables, these are NOT
// derived. A body is a complex three-dimensional shape with no closed-form
// mode solution; real data comes from measuring an actual instrument. The
// values below are built from well-documented landmark resonances (the
// guitar's Helmholtz air mode, the violin's A0 and B1 corpus modes) with
// plausible continuations. They are a starting point for sound design, not an
// emulation of any specific instrument. Treat them like kBellModes: usable,
// but do not present them as physically accurate.
//
// Ratios are relative to each body's LOWEST resonance, so setFundamental()
// takes that resonance in Hz — see kBodyBaseHz below.
// ===========================================================================

// Guitar. Landmarks: Helmholtz air resonance around 100 Hz, top plate T(1,1)
// near 195 Hz, back plate near 215 Hz. Those three dominate the low end and
// are why a dreadnought sounds like one.
static const ModalMode kGuitarBodyModes[] = {
    { 1.00f, 1.000f, 1.00f },   // Helmholtz air
    { 1.95f, 0.850f, 1.00f },   // top plate
    { 2.15f, 0.700f, 1.00f },   // back plate
    { 3.80f, 0.450f, 1.00f },
    { 4.40f, 0.380f, 1.00f },
    { 5.60f, 0.300f, 1.00f },
    { 7.20f, 0.230f, 1.00f },
    { 8.90f, 0.180f, 1.00f },
    { 10.60f, 0.140f, 1.00f },
    { 12.80f, 0.110f, 1.00f },
    { 15.20f, 0.085f, 1.00f },
    { 18.00f, 0.065f, 1.00f },
};

// Violin. Landmarks: A0 air resonance near 275 Hz, B1- and B1+ corpus modes
// near 460 and 530 Hz, and the broad "bridge hill" around 2-3 kHz that gives
// the instrument its projection.
static const ModalMode kViolinBodyModes[] = {
    { 1.00f, 1.000f, 1.00f },   // A0 air
    { 1.67f, 0.900f, 1.00f },   // B1-
    { 1.93f, 0.950f, 1.00f },   // B1+
    { 2.60f, 0.600f, 1.00f },
    { 3.30f, 0.500f, 1.00f },
    { 4.10f, 0.420f, 1.00f },
    { 5.00f, 0.360f, 1.00f },
    { 6.20f, 0.300f, 1.00f },
    { 7.60f, 0.280f, 1.00f },
    { 9.10f, 0.320f, 1.00f },   // bridge hill region: deliberately not falling
    { 10.50f, 0.300f, 1.00f },
    { 12.00f, 0.250f, 1.00f },
    { 14.00f, 0.180f, 1.00f },
    { 16.50f, 0.120f, 1.00f },
};

// Piano soundboard. Large, heavily braced, and dense from very low up — the
// dominant contributor to what a piano actually sounds like.
static const ModalMode kSoundboardModes[] = {
    { 1.00f, 1.000f, 1.00f },
    { 1.40f, 0.880f, 1.00f },
    { 1.80f, 0.780f, 1.00f },
    { 2.30f, 0.680f, 1.00f },
    { 2.90f, 0.600f, 1.00f },
    { 3.40f, 0.530f, 1.00f },
    { 4.10f, 0.470f, 1.00f },
    { 4.80f, 0.410f, 1.00f },
    { 5.60f, 0.360f, 1.00f },
    { 6.50f, 0.320f, 1.00f },
    { 7.50f, 0.280f, 1.00f },
    { 8.70f, 0.245f, 1.00f },
    { 10.00f, 0.215f, 1.00f },
    { 11.50f, 0.190f, 1.00f },
    { 13.20f, 0.165f, 1.00f },
    { 15.10f, 0.145f, 1.00f },
    { 17.30f, 0.125f, 1.00f },
    { 19.80f, 0.110f, 1.00f },
};

// Generic resonant plate. Makes no claim to model anything — a dense, evenly
// inharmonic body for sound design rather than emulation.
static const ModalMode kGenericBodyModes[] = {
    { 1.00f, 1.000f, 1.00f }, { 1.31f, 0.870f, 1.00f },
    { 1.72f, 0.760f, 1.00f }, { 2.19f, 0.660f, 1.00f },
    { 2.74f, 0.580f, 1.00f }, { 3.37f, 0.510f, 1.00f },
    { 4.08f, 0.450f, 1.00f }, { 4.87f, 0.390f, 1.00f },
    { 5.74f, 0.345f, 1.00f }, { 6.69f, 0.305f, 1.00f },
    { 7.72f, 0.270f, 1.00f }, { 8.83f, 0.235f, 1.00f },
    { 10.02f, 0.210f, 1.00f }, { 11.29f, 0.185f, 1.00f },
    { 12.64f, 0.165f, 1.00f }, { 14.07f, 0.145f, 1.00f },
};

// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Shared Stretch value table. Stretch is exposed as an INDEXED parameter so
// two modules (a ModalBank resonator and a ClockGran discretizer aimed at it)
// can be kept in lockstep just by matching the index — continuous floats would
// never line up exactly. 1-2-5 progression per decade, perceptually even.
//
// Negative floors at -0.5: at -1 every mode would collapse onto the fundamental
// (ratio^0). Positive reaches 1.0 (ratio^2), where all but the lowest modes fly
// past Nyquist — extreme-clang territory, included deliberately at the end.
//
// Index 9 is 0.0 (the default). Keep this identical across all copies of this
// header, and in sync with the Swift mirror in ModalBankView.swift.
// ---------------------------------------------------------------------------
static const float kModalStretchValues[] = {
    -0.5f, -0.2f, -0.1f, -0.05f, -0.02f, -0.01f, -0.005f, -0.002f, -0.001f,
     0.0f,
     0.001f, 0.002f, 0.005f, 0.01f, 0.02f, 0.05f, 0.1f, 0.2f, 0.5f, 1.0f,
};
static const int kModalStretchCount     = 20;
static const int kModalStretchZeroIndex = 9;   // value 0.0 == default

enum ModalMaterialID {
    ModalMaterialString = 0,
    ModalMaterialBar,
    ModalMaterialTunedBar,
    ModalMaterialMembrane,
    ModalMaterialOpenTube,
    ModalMaterialClosedTube,
    ModalMaterialTine,
    ModalMaterialBell,

    // Bodies. Downstream of a string bank, at fixed pitch.
    ModalMaterialGuitarBody,
    ModalMaterialViolinBody,
    ModalMaterialSoundboard,
    ModalMaterialGenericBody,

    ModalMaterialCount
};

static const ModalMaterial kModalMaterials[ModalMaterialCount] = {
    { kStringModes,      16, "String"       },
    { kBarModes,          8, "Bar"          },
    { kTunedBarModes,     6, "Tuned Bar"    },
    { kMembraneModes,    16, "Membrane"     },
    { kOpenTubeModes,    12, "Open Tube"    },
    { kClosedTubeModes,  10, "Closed Tube"  },
    { kTineModes,         6, "Tine"         },
    { kBellModes,        12, "Bell"         },
    { kGuitarBodyModes,  12, "Guitar Body"  },
    { kViolinBodyModes,  14, "Violin Body"  },
    { kSoundboardModes,  18, "Soundboard"   },
    { kGenericBodyModes, 16, "Generic Body" },
};

// Lowest resonance of each body, in Hz — pass to setFundamental(). These are
// CONSTANTS, not key-tracked: that is what makes a body a body.
// Index by ModalMaterialID; zero for anything that is not a body.
static const float kBodyBaseHz[ModalMaterialCount] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    100.0f,   // guitar   — Helmholtz air resonance
    275.0f,   // violin   — A0
     70.0f,   // piano    — soundboard low mode
    120.0f,   // generic
};

#endif /* ModalMaterials_hpp */
