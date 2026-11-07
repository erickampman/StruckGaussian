// SoftKnee.hpp — the gentle in/out curve on StruckGaussian's final output (ReleaseUMP R2-26).
// Framework-free.
//
// Ring Fb feeds the output back into the ring mod; only the fed-back sample is clipped
// (ModalBankModule.cpp), so a loud note could reach about (1 + Ring) x its level + 0.98. Each
// voice's FinalModule had a hard ceiling at its clip level (harsh when Ring Fb drives into it), and
// a chord of voices summed past 0 dBFS. The same curve now does both: each voice's ceiling, scaled
// to its clip level, and the plugin's summed output (plugin.cpp), so the host never gets past 0 dBFS:
//   |x| <= k:  unchanged (k = 0.5, -6 dBFS: ordinary playing never touches it)
//   |x| >  k:  k + (1 - k) tanh((|x| - k) / (1 - k))
// Slope 1 at the knee, so there's no corner, and it nears 0 dBFS without reaching it.

#ifndef SoftKnee_hpp
#define SoftKnee_hpp

#include <cmath>
#include <cstdint>

static constexpr float kSoftKnee = 0.5f;

inline float softKnee(float x)
{
    const float a = std::fabs(x);
    if (!(a > kSoftKnee)) return x;   // and NaN passes as it came
    const float y = kSoftKnee + (1.0f - kSoftKnee) * std::tanh((a - kSoftKnee) / (1.0f - kSoftKnee));
    return x < 0.0f ? -y : y;
}

inline void softKnee(float* samples, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i) samples[i] = softKnee(samples[i]);
}

#endif
