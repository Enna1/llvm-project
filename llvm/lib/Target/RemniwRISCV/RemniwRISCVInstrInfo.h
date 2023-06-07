#ifndef LLVM_LIB_TARGET_REMNIWRISCV_REMNIWRISCVINSTRINFO_H
#define LLVM_LIB_TARGET_REMNIWRISCV_REMNIWRISCVINSTRINFO_H

#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "RemniwRISCVGenInstrInfo.inc"

namespace llvm {

class RemniwRISCVSubtarget;

class RemniwRISCVInstrInfo : public RemniwRISCVGenInstrInfo {
  const RemniwRISCVSubtarget &STI;

public:
  explicit RemniwRISCVInstrInfo(RemniwRISCVSubtarget &STI);
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_REMNIWRISCV_REMNIWRISCVINSTRINFO_H
