// RingMod.hpp — what ModalBankModule's ring mod does, for the spectrum view to draw. Framework-free.
//
//    y[n] = x[n] + sin(phi[n]) * (L * x[n] + F * clip(y[n-1]))      L = Ring, F = Ring Fb
//
// phi advances by d = 2 pi fRing / fs a sample. With F = 0 this is plain ring mod added to the
// struck sound: a mode at f keeps its level and gains one pair of sidebands, f +- fRing, each L/2
// of it. F feeds the last output back into the ring mod, and each trip round the one-sample loop
// shifts by fRing again, so sidebands go on to f +- 2 fRing, 3 fRing... For signals slow next to
// the loop, y ~= x (1 + L sin) / (1 - F sin), a geometric series with ratio r = (1 - sqrt(1 - F^2)) / F.
// But near the sine's peak the loop remembers ~1/(1 - F) samples (50 at F = 0.98), as long as an
// audible period, so it also low-passes: high modes and upper sidebands come out well under that
// (at F = 0.98, a 1 kHz mode's first upper sideband is 0.63 of it, not 0.82). So the view solves it
// exactly instead. A mode x = e^{jwn} comes out as y = e^{jwn} * sum_k g_k e^{jk phi}, the clip
// aside, where
//    g_k - c * (g_{k-1} e^{-j(k-1)d} - g_{k+1} e^{-j(k+1)d}) = [k == 0] + (L / 2j) ([k == 1] - [k == -1]),
//    c = F e^{-jw} / 2j
// a tridiagonal system, diagonally dominant (|c| = F/2 < 1/2), so the Thomas algorithm solves it
// stably in O(orders). |g_k| is the component at f + k fRing.
//
// F = 1 puts a pole on the unit circle (and above it the loop grows until the clip, and can keep
// itself going after the note has died), so F is held at kRingFeedbackMax.

#ifndef RingMod_hpp
#define RingMod_hpp

#include <algorithm>
#include <cmath>
#include <complex>

static constexpr float kRingFeedbackMax  = 0.98f;   // Ring Fb's top (StruckGaussianParams.h agrees)
static constexpr float kRingFeedbackClip = 1.0f;    // the fed-back sample is clipped to +-this

// The levels the DSP actually uses.
inline float ringLevelFor(float level)       { return std::clamp(level, 0.0f, 1.0f); }
inline float ringFeedbackFor(float feedback) { return std::clamp(feedback, 0.0f, kRingFeedbackMax); }

// What the ring mod makes of one mode at fHz: mags[K + k] = the amplitude at fHz + k * fRingHz,
// relative to the mode's own, for k = -K..K (k = 0 is the mode itself: 1 without feedback, the
// loop's gain on it with). The system is truncated at K orders; at F = 0.98 order 40 is ~1e-4.
template <int K>
inline void ringSpectrum(float level, float feedback, double fHz, double fRingHz, double fs,
                         float (&mags)[2 * K + 1])
{
    std::fill(std::begin(mags), std::end(mags), 0.0f);
    const double l = ringLevelFor(level), f = ringFeedbackFor(feedback);
    mags[K] = 1.0f;
    if (f <= 0.0) {
        if (K >= 1) mags[K - 1] = mags[K + 1] = (float)(0.5 * l);
        return;
    }

    using C = std::complex<double>;
    constexpr int N = 2 * K + 1;
    const double w = 2.0 * M_PI * fHz / fs, d = 2.0 * M_PI * fRingHz / fs;
    const C c  = f * std::exp(C(0.0, -w)) / C(0.0, 2.0);
    const C lx = l / C(0.0, 2.0);                       // L sin(phi) x: +-1 orders of the input
    // Row i (k = i - K): sub[i] g_{k-1} + g_k + sup[i] g_{k+1} = rhs.
    C sup[N], rhs[N];
    C prevSup = 0.0, prevRhs = 0.0;
    for (int i = 0; i < N; ++i) {                       // forward sweep
        const int k = i - K;
        const C sub = (i > 0)     ? -c * std::exp(C(0.0, -(k - 1) * d)) : C(0.0);
        const C up  = (i < N - 1) ?  c * std::exp(C(0.0, -(k + 1) * d)) : C(0.0);
        const C b   = k == 0 ? C(1.0) : k == 1 ? lx : k == -1 ? -lx : C(0.0);
        const C den = 1.0 - sub * prevSup;
        sup[i] = up / den;
        rhs[i] = (b - sub * prevRhs) / den;
        prevSup = sup[i]; prevRhs = rhs[i];
    }
    C g = rhs[N - 1];                                   // back substitution
    mags[N - 1] = (float)std::abs(g);
    for (int i = N - 2; i >= 0; --i) {
        g = rhs[i] - sup[i] * g;
        mags[i] = (float)std::abs(g);
    }
}

#endif /* RingMod_hpp */
