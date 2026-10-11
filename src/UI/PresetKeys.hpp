// PresetKeys.hpp — the plugin-param ↔ preset-JSON-field mapping, shared by the dev-only JSON
// dumper (PresetDump.hpp) and the always-on preset loader (GeneratedPresets.h resolution in
// StruckSynthUI.cpp). Kept in its own always-compiled header (not gated by STRUCKSYNTH_PRESET_DUMP)
// since preset loading must work in every build, not just dev builds.

#ifndef STRUCKSYNTH_PRESETKEYS_HPP
#define STRUCKSYNTH_PRESETKEYS_HPP

#include "StruckSynthParams.h"

namespace strucksynth {

struct PresetKey { const char* module; const char* item; };

// One entry per param, index-aligned to ParamIndex. Three logical modules: Global voice controls,
// the Strike exciter, and the ModalBank resonator (object / decay / timbre). Items are stable
// preset-file keys — they only need to agree between the dumper and the loader, not with any DSP
// serialization.
static constexpr PresetKey kPresetKeys[kParamCount] = {
   /*VoiceMode*/       {"Global",    "voiceMode"},
   /*Glide*/           {"Global",    "glide"},
   /*Level*/           {"Global",    "level"},
   /*Spread*/          {"Global",    "spread"},
   /*StrikeType*/      {"Strike",    "type"},
   /*Hardness*/        {"Strike",    "hardness"},
   /*VelToLevel*/      {"Strike",    "velToLevel"},
   /*VelToHardness*/   {"Strike",    "velToHardness"},
   /*ContactMode*/     {"Strike",    "contactMode"},
   /*Pitch*/           {"ModalBank", "pitch"},
   /*Material*/        {"ModalBank", "material"},
   /*ModeCount*/       {"ModalBank", "modeCount"},
   /*Stretch*/         {"ModalBank", "stretch"},
   /*DecayT60*/        {"ModalBank", "decayT60"},
   /*DampTilt*/        {"ModalBank", "dampTilt"},
   /*DampBellAmount*/  {"ModalBank", "dampBellAmount"},
   /*DampBellCentre*/  {"ModalBank", "dampBellCentre"},
   /*DampBellWidth*/   {"ModalBank", "dampBellWidth"},
   /*StrikePosition*/  {"ModalBank", "strikePosition"},
   /*Brightness*/      {"ModalBank", "brightness"},
   // MPE describes the controller, not the sound: factory presets leave it out, and the loader keeps
   // the current value for any key a preset does not mention.
   /*Mpe*/             {"Global",    "mpe"},
};

} // namespace strucksynth

#endif // STRUCKSYNTH_PRESETKEYS_HPP
