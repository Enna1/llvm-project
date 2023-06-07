#include "RemniwRISCVInstrInfo.h"
#include "MCTargetDesc/RemniwRISCVMCTargetDesc.h"
#include "RemniwRISCVSubtarget.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "RemniwRISCVGenInstrInfo.inc"

RemniwRISCVInstrInfo::RemniwRISCVInstrInfo(RemniwRISCVSubtarget &STI)
    : RemniwRISCVGenInstrInfo(), STI(STI) {}
