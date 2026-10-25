//
//  ModalCloud.hpp
//
//  A mode set drawn from a Gaussian instead of a series: the struck object's
//  modes as a "cloud" around a centre ratio. Prototyped offline in
//  tools/cloudlab, whose README has the measurements behind every choice here.
//
//  Framework-free, like ModalBank and ModalGenerators: plain arrays, no
//  allocation, safe to call on the audio thread.
//
//    log2(ratio) ~ Normal(centre, width)   octaves above the fundamental
//
//  keepFundamental: mode 0 is the fundamental itself (ratio 1) and the other
//  count-1 are drawn. Without it nothing is sure to sit at the note's pitch.
//
//  Amplitude: Focus morphs between two laws. At 0 it is 1/sqrt(ratio), as for
//  every ModalGenerators series. At 1 it is a second Gaussian, over LOUDNESS:
//
//    amp = exp(-0.5 * ((log2(ratio) - focusCentre) / focusWidth)^2)
//
//  peaked at focusCentre octaves above the fundamental (0 = the fundamental is
//  loudest). In between, amp = (1/sqrt(ratio))^(1-focus) * gauss^focus. Damp is
//  1. Damp Tilt acts on top, as on the other materials. (StruckGaussian turns
//  the bank's Strike Position weighting off and leaves Brightness at 0: both
//  moved the loudness peak, which is Focus's job.)
//
//  RECENTRED: the drawn values are shifted so they average exactly 0, so the
//  cloud's mean log-ratio is exactly `centre` for every seed and count, and
//  Width spreads the modes symmetrically about it. Raw draws average a little
//  off zero (seed 3, 25 modes: -0.43 sigma), so without this the cloud drifted
//  off Centre in proportion to Width, and the pitch moved as Width did. A
//  recentred value outside [kMinRatio, kMaxRatio] is dropped, not redrawn (a
//  redraw would undo the recentring), so a very wide cloud returns fewer
//  modes. The modes come back sorted by ratio, because ModalBank pans by mode
//  index.
//
//  REPEATABLE ON EVERY PLATFORM: the Gaussian is Box-Muller on std::mt19937,
//  whose output the C++ standard fixes exactly. std::normal_distribution is
//  implementation-defined (libc++ and libstdc++ draw different values), so it
//  would give a preset a different object on the Mac than on Linux.
//
//  One seed is one sequence of normal draws, so with the seed fixed a change of
//  width stretches the same cloud about its centre and a change of centre
//  transposes it. A smaller count is the first draws of a larger one,
//  recentred on their own.
//

#ifndef ModalCloud_hpp
#define ModalCloud_hpp

#include "ModalMaterials.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>

namespace ModalCloud {

// 120 is ModalGenerators' kMaxRatio. 1/16 keeps a cloud centred below the note
// from spending modes under about 4 Hz at C2.
constexpr double kMinRatio = 1.0 / 16.0;
constexpr double kMaxRatio = 120.0;

// A uniform in (0, 1) from 53 bits of mt19937 output: never 0, so log() is safe.
inline double uniform01(std::mt19937& g) {
    const uint64_t hi = g() >> 5, lo = g() >> 6;            // 27 + 26 bits
    return ((double)((hi << 26) | lo) + 0.5) / 9007199254740992.0;
}

// Box-Muller, one value per call (the second is discarded: simpler to reason
// about, and a cloud is at most 64 draws).
inline double gaussian(std::mt19937& g) {
    const double u1 = uniform01(g), u2 = uniform01(g);
    return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2);
}

// A mode's amplitude at ratio r: the Focus morph described at the top.
inline float amplitude(double r, double focus, double focusCentre, double focusWidth) {
    const double series = 1.0 / std::sqrt(r);
    if (focus <= 0.0) return (float)series;
    const double d = (std::log2(r) - focusCentre) / std::max(focusWidth, 1.0e-3);
    const double gauss = std::exp(-0.5 * d * d);
    return (float)(std::pow(series, 1.0 - focus) * std::pow(gauss, focus));
}

// Fill `out` with up to `count` modes (clamped to 1..maxOut, at most 64).
// Returns how many: fewer than asked when recentred draws fall outside the range.
inline int generate(int count, double centre, double width, uint32_t seed,
                    bool keepFundamental, ModalMode* out, int maxOut,
                    double focus = 0.0, double focusCentre = 0.0, double focusWidth = 0.5)
{
    const int n = std::clamp(count, 1, maxOut);
    std::mt19937 g(seed);
    int i = 0;
    if (keepFundamental) out[i++] = { 1.0f, amplitude(1.0, focus, focusCentre, focusWidth), 1.0f };
    double z[64];
    const int drawn = std::min(n - i, 64);
    double mean = 0.0;
    for (int k = 0; k < drawn; ++k) { z[k] = gaussian(g); mean += z[k]; }
    if (drawn > 0) mean /= drawn;
    for (int k = 0; k < drawn; ++k) {
        const double r = std::exp2(centre + width * (z[k] - mean));
        if (r < kMinRatio || r > kMaxRatio) continue;       // outside the range: dropped
        out[i++] = { (float)r, amplitude(r, focus, focusCentre, focusWidth), 1.0f };
    }
    std::sort(out, out + i, [](const ModalMode& a, const ModalMode& b) { return a.ratio < b.ratio; });
    return i;
}

} // namespace ModalCloud

#endif /* ModalCloud_hpp */
