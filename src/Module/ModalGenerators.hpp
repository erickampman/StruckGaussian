//
//  ModalGenerators.hpp
//
//  Procedurally generated mode series.
//
//  The tables in ModalMaterials.hpp come from eigenvalue problems and describe
//  objects that exist. These do not. The resonator bank has no opinion on
//  whether a ratio set corresponds to anything physical, so this file explores
//  the rest of the space.
//
//  WHAT ACTUALLY MATTERS PERCEPTUALLY (mathematical elegance does not):
//
//    Fusion      Ratios at or near small-integer relationships fuse into a
//                single perceived pitch. Irrational ratios refuse to fuse and
//                are heard as inharmonic clang. This is the dominant axis.
//
//    Growth      Ratios above Nyquist/f0 are inaudible and get muted by the
//                bank's guard. At f0 = 220 Hz the budget is about 109x, so:
//                  linear (n)        -> 64+ modes usable
//                  quadratic (n^2)   -> ~10 modes
//                  exponential (2^n) -> ~7 modes
//                  factorial/Catalan -> 4-5 modes; not worth generating
//                Growth rate is the hard filter on which series are useful.
//
//    Density     Partials closer than roughly 30 Hz beat against each other.
//                Slow-growing series bunch up and produce roughness, which is
//                a texture rather than a defect — but it is not a pitch.
//
//  Generators return the number of modes actually produced, which may be fewer
//  than requested once the ratio ceiling is hit. Returning the truncated count
//  is more honest than emitting modes that will be silently muted.
//

#ifndef ModalGenerators_hpp
#define ModalGenerators_hpp

#include "ModalMaterials.hpp"
#include <cmath>
#include <algorithm>

enum ModalSeriesID {
    // --- fusing: integer or near-integer ratios -----------------------------
    ModalSeriesHarmonic = 0,   // n            reference: fuses completely
    ModalSeriesOctave,         // 2^n          octave stack, organ mixture
    ModalSeriesFibonacci,      // 1,2,3,5,8    harmonic series with holes
    ModalSeriesPrime,          // 1,2,3,5,7    different holes, hollower
    ModalSeriesTriangular,     // 1,3,6,10     quadratic, sparse
    ModalSeriesTetrahedral,    // 1,4,10,20    NB: starts on the marimba tuning
    ModalSeriesPartition,      // 1,2,3,5,7,11 from the Wikipedia list

    // --- non-fusing: irrational ratios --------------------------------------
    ModalSeriesGolden,         // phi^n        most irrational; never beats
    ModalSeriesSilver,         // (1+sqrt2)^n  sparser cousin of golden

    // --- physically derived, but parameterised ------------------------------
    ModalSeriesPiano,          // n*sqrt(1+Bn^2)  stiff string; param = B

    // --- dense / continuously variable --------------------------------------
    ModalSeriesPower,          // n^k          k=1 harmonic, k=2 ~ bar
    ModalSeriesSqrtN,          // sqrt(n)      slow growth, dense, plate-like
    ModalSeriesBox3D,          // sqrt(i2+j2+k2)  room modes, dense
    ModalSeriesHarmonicNumber, // H_n          logarithmic; maximum beating

    ModalSeriesCount
};

inline const char* modalSeriesName(ModalSeriesID id) {
    switch (id) {
        case ModalSeriesHarmonic:      return "Harmonic (n)";
        case ModalSeriesOctave:        return "Octave (2^n)";
        case ModalSeriesFibonacci:     return "Fibonacci";
        case ModalSeriesPrime:         return "Prime";
        case ModalSeriesTriangular:    return "Triangular";
        case ModalSeriesTetrahedral:   return "Tetrahedral";
        case ModalSeriesPartition:     return "Partition";
        case ModalSeriesPiano:         return "Piano (stiff)";
        case ModalSeriesGolden:        return "Golden (phi^n)";
        case ModalSeriesSilver:        return "Silver";
        case ModalSeriesPower:         return "Power (n^k)";
        case ModalSeriesSqrtN:         return "Sqrt (sqrt n)";
        case ModalSeriesBox3D:         return "Box 3D";
        case ModalSeriesHarmonicNumber:return "Harmonic Number";
        default:                       return "?";
    }
}

namespace ModalGen {

// Ratio ceiling. Beyond this a mode is above Nyquist for any musically useful
// fundamental and would just be muted by the bank.
constexpr float kMaxRatio = 120.0f;

inline bool isPrime(int n) {
    if (n < 2) return false;
    for (int d = 2; (long)d * d <= n; ++d) if (n % d == 0) return false;
    return true;
}

// p(n), number of integer partitions, by the standard DP.
inline void partitions(int upTo, int* out) {
    const int N = upTo + 1;
    // out[i] = p(i); computed with the coin-change style recurrence.
    for (int i = 0; i < N; ++i) out[i] = 0;
    out[0] = 1;
    for (int k = 1; k <= upTo; ++k)
        for (int i = k; i <= upTo; ++i)
            out[i] += out[i - k];
}

} // namespace ModalGen

// ---------------------------------------------------------------------------
// Fill `out` with up to `count` modes. Returns how many were actually written.
//
// `param` is used only by ModalSeriesPower (the exponent k); ignored otherwise.
//
// Amplitude falls as 1/sqrt(ratio) — a mild default that leaves the expressive
// shaping to Brightness and DampTilt rather than baking a curve in here.
// Damping is left flat at 1.0 for the same reason.
// ---------------------------------------------------------------------------
inline int generateModalSeries(ModalSeriesID id, float param,
                               int count, ModalMode* out, int maxOut)
{
    count = std::min(count, maxOut);
    if (count <= 0) return 0;

    float ratios[512];
    int   n = 0;
    const int cap = std::min(count, 512);

    switch (id) {

    case ModalSeriesHarmonic:
        for (int i = 0; i < cap; ++i) ratios[n++] = (float)(i + 1);
        break;

    case ModalSeriesOctave:
        for (int i = 0; i < cap; ++i) {
            const float r = std::pow(2.0f, (float)i);
            if (r > ModalGen::kMaxRatio) break;
            ratios[n++] = r;
        }
        break;

    case ModalSeriesFibonacci: {
        // 1, 2, 3, 5, 8, 13, ... — every term is an integer, so all of them are
        // harmonics of f0. Fuses into a pitch, but with most of the harmonic
        // series missing, which reads as hollow rather than rich.
        long a = 1, b = 2;
        for (int i = 0; i < cap; ++i) {
            if ((float)a > ModalGen::kMaxRatio) break;
            ratios[n++] = (float)a;
            const long t = a + b; a = b; b = t;
        }
        break;
    }

    case ModalSeriesPrime:
        ratios[n++] = 1.0f;
        for (int v = 2; n < cap && v <= (int)ModalGen::kMaxRatio; ++v)
            if (ModalGen::isPrime(v)) ratios[n++] = (float)v;
        break;

    case ModalSeriesTriangular:
        for (int i = 1; n < cap; ++i) {
            const float r = (float)(i * (i + 1) / 2);
            if (r > ModalGen::kMaxRatio) break;
            ratios[n++] = r;
        }
        break;

    case ModalSeriesTetrahedral:
        // 1, 4, 10, 20, 35, ... The first three are exactly the marimba's
        // tuned ratios, which is a coincidence but a memorable one.
        for (int i = 1; n < cap; ++i) {
            const float r = (float)(i * (i + 1) * (i + 2) / 6);
            if (r > ModalGen::kMaxRatio) break;
            ratios[n++] = r;
        }
        break;

    case ModalSeriesPartition: {
        int p[256];
        ModalGen::partitions(64, p);
        for (int i = 1; i <= 64 && n < cap; ++i) {
            const float r = (float)p[i];
            if (r > ModalGen::kMaxRatio) break;
            ratios[n++] = r;
        }
        break;
    }

    case ModalSeriesPiano: {
        // Stiff string. A real piano string resists bending, so it behaves
        // partly like a bar, and the partials go sharp progressively:
        //
        //     f_n = n * f0 * sqrt(1 + B*n^2)
        //
        // B is the inharmonicity coefficient, roughly:
        //     0.00005  bass
        //     0.0003   middle
        //     0.003    treble
        //     0.02     extreme treble (short, thick strings)
        //
        // This is why piano tuning is stretched: partial 2 of a low note is
        // sharp of the upper octave's fundamental, so tuners widen octaves to
        // split the difference.
        //
        // NB this is a genuinely different curve from the bank's Stretch
        // parameter (ratio^(1+s)). Matching partial 2 with a power law leaves
        // partial 10 at 10.07 where the true value is 10.49 — the power law
        // flattens out exactly where the piano keeps climbing. Use this
        // generator rather than trying to dial Stretch to imitate it.
        const float B = (param > 0.0f) ? param : 0.0003f;
        for (int i = 0; i < cap; ++i) {
            const float nn = (float)(i + 1);
            const float r  = nn * std::sqrt(1.0f + B * nn * nn);
            if (r > ModalGen::kMaxRatio) break;
            ratios[n++] = r;
        }
        break;
    }

    case ModalSeriesGolden: {
        // phi is the "most irrational" number — its continued fraction is all
        // 1s, so its powers stay as far from small-integer ratios as possible.
        // No two partials ever approach a simple relationship, so nothing
        // fuses and nothing beats. Should read as shimmering and unpitched.
        const float phi = 1.6180339887f;
        for (int i = 0; i < cap; ++i) {
            const float r = std::pow(phi, (float)i);
            if (r > ModalGen::kMaxRatio) break;
            ratios[n++] = r;
        }
        break;
    }

    case ModalSeriesSilver: {
        const float silver = 2.4142135624f;   // 1 + sqrt(2)
        for (int i = 0; i < cap; ++i) {
            const float r = std::pow(silver, (float)i);
            if (r > ModalGen::kMaxRatio) break;
            ratios[n++] = r;
        }
        break;
    }

    case ModalSeriesPower: {
        // k = 1 is the harmonic series; k = 2 lands close to the free-free bar
        // (1, 4, 9, 16 against the bar's 1, 2.76, 5.40, 8.93). A continuous
        // morph from pitched to clangy, derived rather than hand-tuned.
        const float k = (param > 0.01f) ? param : 1.0f;
        for (int i = 0; i < cap; ++i) {
            const float r = std::pow((float)(i + 1), k);
            if (r > ModalGen::kMaxRatio) break;
            ratios[n++] = r;
        }
        break;
    }

    case ModalSeriesSqrtN:
        // Grows so slowly that 64 modes all sit below ratio 8 — extremely
        // dense in the low register, which is where beating is most audible.
        for (int i = 0; i < cap; ++i) ratios[n++] = std::sqrt((float)(i + 1));
        break;

    case ModalSeriesBox3D: {
        // Standing waves in a rectangular room: f ~ sqrt(i^2 + j^2 + k^2).
        // Dense and inharmonic in a chamber-like way rather than a plate-like
        // one. Normalised so the (1,1,1) mode is the fundamental.
        float all[1024];
        int   m = 0;
        for (int i = 1; i <= 8 && m < 1024; ++i)
            for (int j = 1; j <= 8 && m < 1024; ++j)
                for (int k = 1; k <= 8 && m < 1024; ++k)
                    all[m++] = std::sqrt((float)(i*i + j*j + k*k));

        std::sort(all, all + m);
        const float base = all[0];
        for (int i = 0; i < m && n < cap; ++i) {
            const float r = all[i] / base;
            if (n > 0 && std::fabs(r - ratios[n-1]) < 1e-4f) continue;  // dedupe
            if (r > ModalGen::kMaxRatio) break;
            ratios[n++] = r;
        }
        break;
    }

    case ModalSeriesHarmonicNumber: {
        // H_n = 1 + 1/2 + 1/3 + ... grows like ln(n), so H_64 is only about
        // 4.7. Sixty-four partials crammed under two and a half octaves is
        // maximum roughness — a texture, definitively not a pitch.
        float h = 0.0f;
        for (int i = 1; n < cap; ++i) {
            h += 1.0f / (float)i;
            ratios[n++] = h;
        }
        // Normalise so mode 0 is the fundamental.
        const float base = ratios[0];
        for (int i = 0; i < n; ++i) ratios[i] /= base;
        break;
    }

    default:
        return 0;
    }

    for (int i = 0; i < n; ++i) {
        out[i].ratio = ratios[i];
        out[i].amp   = 1.0f / std::sqrt(ratios[i]);
        out[i].damp  = 1.0f;
    }
    return n;
}

// ---------------------------------------------------------------------------
// Unison expansion.
//
// A piano note has one, two or three strings tuned ALMOST identically. Because
// the modes of a bank are independent resonators, three detuned banks are
// mathematically identical to one bank holding three detuned copies of every
// mode. Unison therefore needs no architectural support — it is a transform on
// the mode list, and it costs `copies` times as many modes, which is
// affordable given how cheap modes turned out to be.
//
// Two effects come out of this:
//
//   Beating.       Copies a cent or two apart drift in and out of phase,
//                  which is the "depth" or chorusing.
//
//   Double decay.  Real strings couple through the bridge: they start in
//                  phase (strong coupling, rapid energy loss) and drift out of
//                  phase (motion cancels at the bridge, slow loss), giving the
//                  piano's characteristic fast "prompt" followed by a long
//                  "aftersound". True coupling is not modelled here, but
//                  giving the copies DIFFERENT decay rates approximates the
//                  two-stage envelope closely enough to be convincing.
//
// detuneCents is the total spread across the copies; 0.5 to 2 cents is the
// realistic range. Much beyond that stops sounding like one note.
// ---------------------------------------------------------------------------
inline int expandUnison(const ModalMode* in, int inCount,
                        int copies, float detuneCents, float decaySpread,
                        ModalMode* out, int maxOut)
{
    if (copies < 1) copies = 1;
    int n = 0;

    for (int i = 0; i < inCount; ++i) {
        for (int c = 0; c < copies && n < maxOut; ++c) {
            // Spread symmetrically about the nominal pitch.
            const float offset = (copies > 1)
                ? ((float)c / (float)(copies - 1) - 0.5f) * detuneCents
                : 0.0f;

            // One copy rings noticeably longer than the others. That asymmetry
            // is what produces the aftersound; equal decays would only chorus.
            const float damp = (copies > 1)
                ? in[i].damp * (1.0f - decaySpread * 0.5f
                                + decaySpread * (float)c / (float)(copies - 1))
                : in[i].damp;

            out[n].ratio = in[i].ratio * std::pow(2.0f, offset / 1200.0f);
            out[n].amp   = in[i].amp / (float)copies;
            out[n].damp  = std::fmax(damp, 0.05f);
            ++n;
        }
    }
    return n;
}

#endif /* ModalGenerators_hpp */
