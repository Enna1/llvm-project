#ifndef LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCVSUBTARGET_H
#define LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCVSUBTARGET_H

#include "LtumnfRISCVFrameLowering.h"
#include "LtumnfRISCVISelLowering.h"
#include "LtumnfRISCVInstrInfo.h"
#include "LtumnfRISCVRegisterInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"

#define GET_SUBTARGETINFO_HEADER
#include "LtumnfRISCVGenSubtargetInfo.inc"

namespace llvm {

class LtumnfRISCVTargetMachine;

class LtumnfRISCVSubtarget : public LtumnfRISCVGenSubtargetInfo {
  LtumnfRISCVTargetLowering TLInfo;
  LtumnfRISCVFrameLowering FrameLowering;
  LtumnfRISCVInstrInfo InstrInfo;
  LtumnfRISCVRegisterInfo RegInfo;

public:
  LtumnfRISCVSubtarget(const Triple &TT, StringRef CPU, StringRef TuneCPU,
                       StringRef FS, const TargetMachine &TM);

  const LtumnfRISCVTargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }

  const LtumnfRISCVFrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }

  const LtumnfRISCVInstrInfo *getInstrInfo() const override {
    return &InstrInfo;
  }

  const LtumnfRISCVRegisterInfo *getRegisterInfo() const override {
    return &RegInfo;
  }

  MVT getXLenVT() const { return MVT::i64; }

private:
  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  LtumnfRISCVSubtarget &initializeSubtargetDependencies(const Triple &TT,
                                                        StringRef CPU,
                                                        StringRef FS);
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCVSUBTARGET_H
