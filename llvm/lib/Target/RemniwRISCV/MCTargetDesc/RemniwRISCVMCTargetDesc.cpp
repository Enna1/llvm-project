#include "RemniwRISCVMCTargetDesc.h"
#include "RemniwRISCVInstPrinter.h"
#include "RemniwRISCVMCAsmInfo.h"
#include "TargetInfo/RemniwRISCVTargetInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

#define GET_INSTRINFO_MC_DESC
#include "RemniwRISCVGenInstrInfo.inc"

#define GET_REGINFO_MC_DESC
#include "RemniwRISCVGenRegisterInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "RemniwRISCVGenSubtargetInfo.inc"

using namespace llvm;

namespace {

MCInstrInfo *createRemniwRISCVMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitRemniwRISCVMCInstrInfo(X);
  return X;
}

MCRegisterInfo *createRemniwRISCVMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitRemniwRISCVMCRegisterInfo(X, RemniwRISCV::X1);
  return X;
}

MCAsmInfo *createRemniwRISCVMCAsmInfo(const MCRegisterInfo &MRI,
                                      const Triple &TT,
                                      const MCTargetOptions &Options) {
  return new RemniwRISCVMCAsmInfo(TT);
}

MCSubtargetInfo *createRemniwRISCVMCSubtargetInfo(const Triple &TT,
                                                  StringRef CPU, StringRef FS) {
  if (CPU.empty()) {
    assert(TT.isArch64Bit() && "Only RV64 is currently supported!");
    CPU = "generic-rv64";
  }
  return createRemniwRISCVMCSubtargetInfoImpl(TT, CPU, CPU, FS);
}

MCInstPrinter *createRemniwRISCVMCInstPrinter(const Triple &T,
                                              unsigned SyntaxVariant,
                                              const MCAsmInfo &MAI,
                                              const MCInstrInfo &MII,
                                              const MCRegisterInfo &MRI) {
  return new RemniwRISCVInstPrinter(MAI, MII, MRI);
}

} // end anonymous namespace

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRemniwRISCVTargetMC() {
  Target &RV64 = getRemniwRISCV64Target();
  TargetRegistry::RegisterMCAsmInfo(RV64, createRemniwRISCVMCAsmInfo);
  TargetRegistry::RegisterMCInstrInfo(RV64, createRemniwRISCVMCInstrInfo);
  TargetRegistry::RegisterMCRegInfo(RV64, createRemniwRISCVMCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(RV64,
                                          createRemniwRISCVMCSubtargetInfo);
  TargetRegistry::RegisterMCInstPrinter(RV64, createRemniwRISCVMCInstPrinter);
}
