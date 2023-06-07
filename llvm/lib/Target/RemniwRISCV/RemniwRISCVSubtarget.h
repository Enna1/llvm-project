#ifndef LLVM_LIB_TARGET_REMNIWRISCV_REMNIWRISCVSUBTARGET_H
#define LLVM_LIB_TARGET_REMNIWRISCV_REMNIWRISCVSUBTARGET_H

#include "RemniwRISCVFrameLowering.h"
#include "RemniwRISCVISelLowering.h"
#include "RemniwRISCVInstrInfo.h"
#include "RemniwRISCVRegisterInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"

#define GET_SUBTARGETINFO_HEADER
#include "RemniwRISCVGenSubtargetInfo.inc"

namespace llvm {

class RemniwRISCVTargetMachine;

class RemniwRISCVSubtarget : public RemniwRISCVGenSubtargetInfo {
  RemniwRISCVTargetLowering TLInfo;
  RemniwRISCVFrameLowering FrameLowering;
  RemniwRISCVInstrInfo InstrInfo;
  RemniwRISCVRegisterInfo RegInfo;

public:
  RemniwRISCVSubtarget(const Triple &TT, StringRef CPU, StringRef TuneCPU,
                       StringRef FS, RemniwRISCVTargetMachine &TM);

  const RemniwRISCVTargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }

  const RemniwRISCVFrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }

  const RemniwRISCVInstrInfo *getInstrInfo() const override {
    return &InstrInfo;
  }

  const RemniwRISCVRegisterInfo *getRegisterInfo() const override {
    return &RegInfo;
  }

private:
  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  RemniwRISCVSubtarget &initializeSubtargetDependencies(const Triple &TT,
                                                        StringRef CPU,
                                                        StringRef FS);
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_REMNIWRISCV_REMNIWRISCVSUBTARGET_H
