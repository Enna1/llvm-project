#include "LtumnfRISCVMCTargetDesc.h"
#include "LtumnfRISCVInstPrinter.h"
#include "LtumnfRISCVMCAsmInfo.h"
#include "TargetInfo/LtumnfRISCVTargetInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

#define GET_INSTRINFO_MC_DESC
#include "LtumnfRISCVGenInstrInfo.inc"

#define GET_REGINFO_MC_DESC
#include "LtumnfRISCVGenRegisterInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "LtumnfRISCVGenSubtargetInfo.inc"

using namespace llvm;

namespace {

MCInstrInfo *createLtumnfRISCVMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitLtumnfRISCVMCInstrInfo(X);
  return X;
}

MCRegisterInfo *createLtumnfRISCVMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitLtumnfRISCVMCRegisterInfo(X, LtumnfRISCV::X1);
  return X;
}

MCAsmInfo *createLtumnfRISCVMCAsmInfo(const MCRegisterInfo &MRI,
                                      const Triple &TT,
                                      const MCTargetOptions &Options) {
  return new LtumnfRISCVMCAsmInfo(TT);
}

MCSubtargetInfo *createLtumnfRISCVMCSubtargetInfo(const Triple &TT,
                                                  StringRef CPU, StringRef FS) {
  if (CPU.empty()) {
    assert(TT.isArch64Bit() && "Only RV64 is currently supported!");
    CPU = "generic-rv64";
  }
  return createLtumnfRISCVMCSubtargetInfoImpl(TT, CPU, CPU, FS);
}

MCInstPrinter *createLtumnfRISCVMCInstPrinter(const Triple &T,
                                              unsigned SyntaxVariant,
                                              const MCAsmInfo &MAI,
                                              const MCInstrInfo &MII,
                                              const MCRegisterInfo &MRI) {
  return new LtumnfRISCVInstPrinter(MAI, MII, MRI);
}

} // end anonymous namespace

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeLtumnfRISCVTargetMC() {
  Target &RV64 = getLtumnfRISCV64Target();
  TargetRegistry::RegisterMCAsmInfo(RV64, createLtumnfRISCVMCAsmInfo);
  TargetRegistry::RegisterMCInstrInfo(RV64, createLtumnfRISCVMCInstrInfo);
  TargetRegistry::RegisterMCRegInfo(RV64, createLtumnfRISCVMCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(RV64,
                                          createLtumnfRISCVMCSubtargetInfo);
  TargetRegistry::RegisterMCInstPrinter(RV64, createLtumnfRISCVMCInstPrinter);
}
