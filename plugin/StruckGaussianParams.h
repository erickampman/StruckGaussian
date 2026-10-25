// StruckGaussianParams.h — the plugin's parameter table, shared by the DSP and UI. A focused modal
// set: global voice controls, the Strike exciter, and the ModalBank resonator (object / decay /
// timbre / output). Non-global params route to the matching VoiceManager module item (see the
// DSP's vmAddress). Defaults mirror the module constants in pm_param_data.cpp.

#ifndef STRUCKGAUSSIAN_PARAMS_H
#define STRUCKGAUSSIAN_PARAMS_H

#include <cstddef>
#include <cstdint>

namespace struckgaussian {

enum ParamIndex {
   // --- Global ---
   kParamVoiceMode = 0, // 0 = Mono (1 voice + glide), 1..3 = Poly 10 / 20 / 32
   kParamGlide,         // pitch glide — drives the Slew module's Rise and Fall together (mono)
   kParamLevel,         // master level → ModalBank Level
   kParamSpread,        // stereo distribution of modes → ModalBank Spread
   // --- Strike (exciter) ---
   kParamStrikeType,        // 0 = Mallet, 1 = Noise burst
   kParamHardness,          // 0 = hard/bright 1-sample impulse .. 1 = soft/dark
   kParamVelToLevel,        // velocity → loudness
   kParamVelToHardness,     // velocity → brighter/shorter contact
   kParamContactMode,       // 0 = Note-Relative, 1 = Wall-Clock
   // --- ModalBank: object ---
   kParamPitch,             // transpose offset, octaves
   kParamCloudCentre,       // the mode cloud's centre, octaves above the note (0 = on the note)
   kParamCloudWidth,        // the cloud's spread (Gaussian sigma), octaves
   kParamModeCount,         // number of modes in the cloud
   kParamCloudSeed,         // which cloud: one seed is one object
   kParamCloudReseed,       // 0 = Off (the seed's cloud every note), 1 = a new cloud per note
   kParamCloudKeepFund,     // 1 = mode 0 is the note's fundamental, 0 = every mode is drawn
   kParamStretch,           // inharmonicity index (into kModalStretchValues)
   // --- ModalBank: decay ---
   kParamDecayT60,          // T60 of mode 0, seconds
   kParamDampTilt,          // HF damping (0 metal .. ~2 wood)
   // --- ModalBank: timbre (Strike Position and Brightness were removed: they moved the loudness
   //     peak, which is Focus's job; the bank's strike-position weighting is off, brightness 0) ---
   kParamFocus,             // 0 = amplitude 1/sqrt(ratio), 1 = a Gaussian loudness curve (replaced the damp bell)
   kParamFocusCentre,       // the loudness peak, octaves above the note (0 = the fundamental loudest)
   kParamFocusWidth,        // the loudness curve's width, octaves
   // --- Appended (CLAP ids are indexes, so new params go last once released; the cloud's params
   //     were put in panel order above while StruckGaussian was unreleased) ---
   kParamMpe,               // MPE input, lower zone: member bend ±48 + the master channel's bend
   // --- Expression (appended): what pressure and timbre (CC74) do; all 0 = no effect ---
   kParamPressToDecay,      // pressure → decay time, octaves of T60 at full pressure (+ rings longer)
   kParamTimbreToTilt,      // timbre → damping tilt, added to Damp at full timbre (+ darker tail)
   kParamCount
};

struct ParamSpec {
   const char* name;
   const char* symbol;
   const char* unit;
   float       min;
   float       max;
   float       def;
   bool        logarithmic;
   bool        integer;
};

static constexpr ParamSpec kParams[kParamCount] = {
   // Global
   {"Voices",     "voices",   "",    0.0f,   3.0f,   1.0f,  false, true },
   {"Glide",      "glide",    "s",   0.0001f, 2.0f,  0.01f, true,  false},
   {"Level",      "level",    "",    0.0f,   2.0f,   0.5f,  false, false},
   {"Spread",     "spread",   "",    0.0f,   1.0f,   0.3f,  false, false},
   // Strike
   {"Strike",     "strkType", "",    0.0f,   1.0f,   0.0f,  false, true },
   {"Hardness",   "hardness", "",    0.0f,   1.0f,   0.3f,  false, false},
   {"Vel>Level",  "velLevel", "",    0.0f,   1.0f,   1.0f,  false, false},
   {"Vel>Hard",   "velHard",  "",    0.0f,   1.0f,   0.5f,  false, false},
   {"Contact",    "contact",  "",    0.0f,   1.0f,   0.0f,  false, true },
   // ModalBank: object
   {"Pitch",      "pitch",    "oct", -4.0f,  4.0f,   0.0f,  false, false},
   {"Centre",     "centre",   "oct", -2.0f,  4.0f,   0.0f,  false, false},
   {"Width",      "width",    "oct", 0.002f, 3.0f,   0.5f,  true,  false},
   {"Modes",      "modes",    "",    1.0f,  64.0f,  16.0f,  false, true },
   {"Seed",       "seed",     "",    1.0f, 100.0f,   1.0f,  false, true },
   {"Reseed",     "reseed",   "",    0.0f,   1.0f,   0.0f,  false, true },
   {"Keep Fund",  "keepFund", "",    0.0f,   1.0f,   1.0f,  false, true },
   {"Stretch",    "stretch",  "",    0.0f,  19.0f,   9.0f,  false, true },
   // ModalBank: decay
   {"Decay",      "decay",    "s",   0.02f, 60.0f,   2.0f,  true,  false},
   {"Damp",       "dampTilt", "",    0.0f,   3.0f,   0.9f,  false, false},
   // ModalBank: timbre
   {"Focus",      "focus",    "",    0.0f,   1.0f,   0.0f,  false, false},
   {"Focus Ctr",  "focusCtr", "oct", -2.0f,  4.0f,   0.0f,  false, false},
   {"Focus Wid",  "focusWid", "oct", 0.05f,  3.0f,   0.5f,  true,  false},
   // Appended
   {"MPE",        "mpe",      "",    0.0f,   1.0f,   0.0f,  false, true },
   // Expression
   {"Press>Decay", "prsDecay", "oct", -4.0f, 4.0f,   0.0f,  false, false},
   {"Tmb>Tilt",   "tmbTilt",  "",    -3.0f,  3.0f,   0.0f,  false, false},
};

// kParams is indexed by ParamIndex, so a swapped pair of rows compiles fine and silently cross-wires
// two controls (PlainOldSynth's Effect Mix and Spread once did exactly that). The table is small, so
// check every row's symbol against the enum rather than spot-checking.
constexpr bool symbolIs(ParamIndex index, const char* symbol)
{
   const char* a = kParams[index].symbol;
   while (*a != '\0' && *a == *symbol) { ++a; ++symbol; }
   return *a == *symbol;
}
static_assert(symbolIs(kParamVoiceMode, "voices") && symbolIs(kParamGlide, "glide")
              && symbolIs(kParamLevel, "level") && symbolIs(kParamSpread, "spread")
              && symbolIs(kParamStrikeType, "strkType") && symbolIs(kParamHardness, "hardness")
              && symbolIs(kParamVelToLevel, "velLevel") && symbolIs(kParamVelToHardness, "velHard")
              && symbolIs(kParamContactMode, "contact") && symbolIs(kParamPitch, "pitch")
              && symbolIs(kParamCloudCentre, "centre") && symbolIs(kParamCloudWidth, "width")
              && symbolIs(kParamModeCount, "modes") && symbolIs(kParamCloudSeed, "seed")
              && symbolIs(kParamCloudReseed, "reseed") && symbolIs(kParamCloudKeepFund, "keepFund")
              && symbolIs(kParamStretch, "stretch") && symbolIs(kParamDecayT60, "decay")
              && symbolIs(kParamDampTilt, "dampTilt")
              && symbolIs(kParamFocus, "focus") && symbolIs(kParamFocusCentre, "focusCtr")
              && symbolIs(kParamFocusWidth, "focusWid")
              && symbolIs(kParamMpe, "mpe") && symbolIs(kParamPressToDecay, "prsDecay")
              && symbolIs(kParamTimbreToTilt, "tmbTilt"),
              "kParams rows are out of ParamIndex order");

// --- dropdowns -------------------------------------------------------------------------------
// One source of truth for "is this parameter stepped-with-labels, and what are the labels": the
// CLAP params extension (CLAP_PARAM_IS_ENUM + value/text conversion) and the UI dropdowns both read
// it from here, so a label can never drift between the host's generic panel and our own.

static constexpr const char* kVoiceModeChoices[]   = { "Mono", "Poly 10", "Poly 20", "Poly 32", nullptr };
static constexpr const char* kStrikeTypeChoices[]  = { "Mallet", "Noise",   nullptr };
static constexpr const char* kContactModeChoices[] = { "Note-Rel", "Wall-Clk", nullptr };
static constexpr const char* kMpeChoices[]         = { "Off", "On",         nullptr };
static constexpr const char* kReseedChoices[]      = { "Off", "Per Note",   nullptr };
static constexpr const char* kKeepFundChoices[]    = { "No",  "Yes",        nullptr };

inline const char* const* choicesFor(uint32_t index)
{
   switch (index) {
   case kParamVoiceMode:   return kVoiceModeChoices;
   case kParamStrikeType:  return kStrikeTypeChoices;
   case kParamContactMode: return kContactModeChoices;
   case kParamMpe:         return kMpeChoices;
   case kParamCloudReseed: return kReseedChoices;
   case kParamCloudKeepFund: return kKeepFundChoices;
   default:                return nullptr;
   }
}

inline bool isDropdownParam(uint32_t index) { return choicesFor(index) != nullptr; }

} // namespace struckgaussian

#endif // STRUCKGAUSSIAN_PARAMS_H
