#include "RemniwRISCVTargetMachine.h"
#include "RemniwRISCV.h"
#include "TargetInfo/RemniwRISCVTargetInfo.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

namespace {

class RemniwRISCVPassConfig : public TargetPassConfig {
public:
  RemniwRISCVPassConfig(RemniwRISCVTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  bool addInstSelector() override {
    addPass(
        createRemniwRISCVISelDag(getRemniwRISCVTargetMachine(), getOptLevel()));
    return false;
  }

private:
  RemniwRISCVTargetMachine &getRemniwRISCVTargetMachine() const {
    return getTM<RemniwRISCVTargetMachine>();
  }
};

StringRef computeDataLayout(const Triple &TT) {
  assert(TT.isArch64Bit() && "Only RV64 is currently supported!");
  return "e-m:e-p:64:64-i64:64-i128:128-n32:64-S128";
}

Reloc::Model getEffectiveRelocModel(std::optional<Reloc::Model> RM) {
  return RM.has_value() ? *RM : Reloc::Static;
}

} // end anonymous namespace

RemniwRISCVTargetMachine::RemniwRISCVTargetMachine(
    const Target &T, const Triple &TT, StringRef CPU, StringRef FS,
    const TargetOptions &Options, std::optional<Reloc::Model> RM,
    std::optional<CodeModel::Model> CM, CodeGenOpt::Level OL, bool JIT)
    : LLVMTargetMachine(T, computeDataLayout(TT), TT, CPU, FS, Options,
                        getEffectiveRelocModel(RM),
                        getEffectiveCodeModel(CM, CodeModel::Small), OL),
      Subtarget(TT, CPU, CPU, FS, *this),
      TLOF(std::make_unique<TargetLoweringObjectFileELF>()) {
  initAsmInfo();
}

TargetPassConfig *
RemniwRISCVTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new RemniwRISCVPassConfig(*this, PM);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRemniwRISCVTarget() {
  RegisterTargetMachine<RemniwRISCVTargetMachine> X(getRemniwRISCV64Target());
}
