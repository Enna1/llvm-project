#include "RemniwRISCVSubtarget.h"
#include "RemniwRISCVTargetMachine.h"

using namespace llvm;

#define DEBUG_TYPE "riscv-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "RemniwRISCVGenSubtargetInfo.inc"

RemniwRISCVSubtarget::RemniwRISCVSubtarget(const Triple &TT, StringRef CPU,
                                           StringRef TuneCPU, StringRef FS,
                                           RemniwRISCVTargetMachine &TM)
    : RemniwRISCVGenSubtargetInfo(TT, CPU, TuneCPU, FS), TLInfo(TM),
      FrameLowering(initializeSubtargetDependencies(TT, CPU, FS)),
      InstrInfo(*this), RegInfo(getHwMode()) {}

RemniwRISCVSubtarget &RemniwRISCVSubtarget::initializeSubtargetDependencies(
    const Triple &TT, StringRef CPU, StringRef FS) {
  if (CPU.empty()) {
    assert(TT.isArch64Bit() && "Only RV64 is currently supported!");
    CPU = "generic-rv64";
  }
  ParseSubtargetFeatures(CPU, CPU, FS);
  return *this;
}
