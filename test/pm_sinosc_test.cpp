// pm_sinosc_test — runs the real vendored SinOscillator DSP off Apple. With no pitch input
// connected it must produce an ~A440 sine. Verifies oscillation, amplitude, and frequency.

#include "Module.hpp"
#include "PitchConverter.hpp"
#include "SinOscillatorModule.hpp"
#include "pm_param_data.h"
#include "pm_params.h"

#include <cmath>
#include <cstdio>

static int g_fails = 0;
#define CHECK(cond, msg)                                                                           \
   do {                                                                                            \
      if (!(cond)) {                                                                               \
         std::printf("FAIL: %s\n", (msg));                                                         \
         ++g_fails;                                                                                \
      }                                                                                            \
   } while (0)

int main() {
   const double sr = 48000.0;
   const uint32_t N = 512;
   const AUParameterAddress id = MODULE_PARAM_ADDRESS(ModuleParamKindSinOsc, ModuleInstance1, 0);

   // Construct with the real SinOsc procs; the constructor runs InitSinOscillator.
   Module m(id, sr, N, &sinOscConstantsGroup, InitSinOscillator, DeinitSinOscillator,
            ProcessSinOscillator, MIDISinOscillator, nullptr, ResetSinOscillator,
            SampleRateSinOscillator);
   m.addParams(&sinOscConstantsGroup);
   PitchConverter pc(440.0);
   m.setPitchConverter(&pc);

   // Render a couple of blocks (first block starts at phase 0).
   ProcessSinOscillator(1, N, &m);
   ProcessSinOscillator(2, N, &m);
   const float* out = m.output.first->data;

   int   crossings = 0;
   float lo = 1e9f, hi = -1e9f;
   for (uint32_t i = 1; i < N; ++i) {
      if ((out[i - 1] <= 0.0f) != (out[i] <= 0.0f))
         ++crossings;
      lo = std::fmin(lo, out[i]);
      hi = std::fmax(hi, out[i]);
   }
   const double freq = (crossings / 2.0) * (sr / N);

   CHECK(hi > 0.9f && lo < -0.9f, "oscillates near full scale");
   CHECK(hi <= 1.01f && lo >= -1.01f, "bounded to [-1,1]");
   CHECK(freq > 380.0 && freq < 500.0, "frequency ~ A440");

   std::printf("SinOsc: freq~%.1f Hz, amp[%.3f, %.3f], crossings=%d\n", freq, lo, hi, crossings);

   if (g_fails == 0)
      std::printf("ALL PASS\n");
   return g_fails ? 1 : 0;
}
