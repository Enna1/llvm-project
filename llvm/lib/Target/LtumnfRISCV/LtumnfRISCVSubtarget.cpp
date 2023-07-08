#include "LtumnfRISCVSubtarget.h"
#include "LtumnfRISCVTargetMachine.h"

using namespace llvm;

#define DEBUG_TYPE "riscv-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "LtumnfRISCVGenSubtargetInfo.inc"

LtumnfRISCVSubtarget::LtumnfRISCVSubtarget(const Triple &TT, StringRef CPU,
                                           StringRef TuneCPU, StringRef FS,
                                           const TargetMachine &TM)
    : LtumnfRISCVGenSubtargetInfo(TT, CPU, TuneCPU, FS), TLInfo(TM, *this),
      FrameLowering(initializeSubtargetDependencies(TT, CPU, FS)),
      InstrInfo(*this), RegInfo(getHwMode()) {}

LtumnfRISCVSubtarget &LtumnfRISCVSubtarget::initializeSubtargetDependencies(
    const Triple &TT, StringRef CPU, StringRef FS) {
  if (CPU.empty()) {
    assert(TT.isArch64Bit() && "Only RV64 is currently supported!");
    CPU = "generic-rv64";
  }
  ParseSubtargetFeatures(CPU, CPU, FS);
  return *this;
}
