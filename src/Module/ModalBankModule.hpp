//
//  ModalBankModule.hpp
//
//  PolygraphModular wrapper around the framework-free ModalBank core
//  (ModalBank.hpp/.cpp, ModalMaterials.hpp). The wrapper does three things and
//  nothing else: marshal PM parameters into the core's setters, convert PM's
//  octave pitch to the Hz the core wants, and feed the excitation port through
//  processBlock. All DSP lives in the core so the CLAP build can reuse it
//  verbatim. See ModalSynthesis.md at the repo root.
//

#ifndef ModalBankModule_hpp
#define ModalBankModule_hpp

#include "Module.hpp"

extern const void        InitModalBankModule(uint32_t count, Module* module);
extern const void        DeinitModalBankModule(Module* module);
extern const void        ResetModalBankModule(Module* module);
extern const void        SampleRateModalBankModule(Module* module, double newSampleRate);
extern const KPBuffPair* ProcessModalBankModule(uint64_t processCallID, uint32_t count, Module* module);

#endif /* ModalBankModule_hpp */
