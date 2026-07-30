//
//  ModalDiscretizer.hpp
//
//  Shared logic for quantising a granular module's grain pitch onto a modal
//  resonator's mode lattice (ModalSynthesis.md, "quantise pitch scatter to the
//  resonator's mode ratios"). Used by ClockGran and IFSGran.
//
//  This is SHARED CODE, NOT SHARED STATE. Both functions are pure; every module
//  keeps its own ratio cache and its own RNG and calls these. No two module
//  instances ever touch the same memory.
//
//  Framework-free (only ModalMaterials/ModalGenerators), so it travels with the
//  core to the CLAP build like the rest of the ModalBank code.
//

#ifndef ModalDiscretizer_hpp
#define ModalDiscretizer_hpp

#include "ModalMaterials.hpp"
#include "ModalGenerators.hpp"
#include <cmath>
#include <algorithm>

// Quantise mode (the QuantiseMode parameter). Snap keeps the source pitch
// distribution and lands each grain on its nearest mode; Scatter draws a mode
// near a chosen centre. Order MUST match kQuantiseModeLabels in
// ModalDiscretizerControls.swift.
enum ModalQuantiseMode {
    kModalQuantiseSnap    = 0,
    kModalQuantiseScatter = 1,
};

// Fill `out` with the mode ratios of material `idx` (ModalBank material space:
// 0..11 static tables/bodies, 12+ procedural generators), with Stretch applied
// as f_i = ratio^(1+stretch) — the SAME warp ModalBank uses, so setting both
// modules to the same material AND stretch index couples them exactly. Returns
// the count. Piano/Power generators use the module-side fixed defaults.
inline int modalDiscretizerRatios(int idx, float stretch, float* out, int maxOut)
{
    if (idx < 0) return 0;
    const float e = 1.0f + stretch;
    int n = 0;
    if (idx < ModalMaterialCount) {              // static table or body
        const ModalMaterial& m = kModalMaterials[idx];
        n = std::min(m.count, maxOut);
        for (int i = 0; i < n; ++i) out[i] = std::pow(m.modes[i].ratio, e);
        return n;
    }
    const ModalSeriesID series = (ModalSeriesID)(idx - ModalMaterialCount);
    const float param = (series == ModalSeriesPiano) ? 0.0003f
                      : (series == ModalSeriesPower) ? 1.5f : 0.0f;
    ModalMode tmp[64];
    n = generateModalSeries(series, param, std::min(maxOut, 64), tmp, 64);
    for (int i = 0; i < n; ++i) out[i] = std::pow(tmp[i].ratio, e);
    return n;
}

// SNAP an existing pitch (in semitones relative to the fundamental) to the
// nearest mode of the lattice, returning the snapped semitone value. This is
// the modal analogue of EDO quantisation: the source distribution (an attractor
// cloud, a scatter jitter) keeps its shape, but every pitch lands on a real
// mode. `ratios` are the post-stretch ratios; `count` their number.
inline float modalDiscretizerSnapSemis(float pitchSemis, const float* ratios, int count)
{
    if (count <= 0) return pitchSemis;
    int   best = 0;
    float bestDist = 1e30f;
    for (int i = 0; i < count; ++i) {
        const float modeSemis = 12.0f * std::log2(std::max(ratios[i], 1e-6f));
        const float d = std::fabs(pitchSemis - modeSemis);
        if (d < bestDist) { bestDist = d; best = i; }
    }
    return 12.0f * std::log2(std::max(ratios[best], 1e-6f));
}

// Pick a mode index near `centre`, scattered by +/- `scatter` in INDEX space
// (bidirectional by construction — the whole point, since ratios are >= 1 and
// pitch scatter would be one-sided). `skew` biases toward low (-1) or high (+1)
// modes. `r1`/`r2` are two independent uniform [0,1) draws supplied by the
// caller, so each module keeps its own RNG and sequence. Result is clamped to
// [0, count-1].
inline int modalDiscretizerPickIndex(int centre, int scatter, float skew,
                                     int count, float r1, float r2)
{
    if (count <= 0) return 0;
    int idx = centre;
    if (scatter > 0) {
        const float u = r1 + r2 - 1.0f;                 // -1..1, centre-weighted
        const float skewed = u + skew * (1.0f - std::fabs(u));
        idx += (int)std::lround(skewed * (float)scatter);
    }
    return std::clamp(idx, 0, count - 1);
}

#endif /* ModalDiscretizer_hpp */
