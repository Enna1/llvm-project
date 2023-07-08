#include "LtumnfRISCVTargetMachine.h"
#include "LtumnfRISCV.h"
#include "TargetInfo/LtumnfRISCVTargetInfo.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

static StringRef computeDataLayout(const Triple &TT) {
  assert(TT.isArch64Bit() && "Only RV64 is currently supported!");
  return "e-m:e-p:64:64-i64:64-i128:128-n32:64-S128";
}

static Reloc::Model getEffectiveRelocModel(std::optional<Reloc::Model> RM) {
  return RM.value_or(Reloc::Static);
}

LtumnfRISCVTargetMachine::LtumnfRISCVTargetMachine(
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

namespace {
class LtumnfRISCVPassConfig : public TargetPassConfig {
public:
  LtumnfRISCVPassConfig(LtumnfRISCVTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  bool addInstSelector() override {
    addPass(
        createLtumnfRISCVISelDag(getLtumnfRISCVTargetMachine(), getOptLevel()));
    return false;
  }

private:
  LtumnfRISCVTargetMachine &getLtumnfRISCVTargetMachine() const {
    return getTM<LtumnfRISCVTargetMachine>();
  }
};

} // end anonymous namespace

TargetPassConfig *
LtumnfRISCVTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new LtumnfRISCVPassConfig(*this, PM);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeLtumnfRISCVTarget() {
  RegisterTargetMachine<LtumnfRISCVTargetMachine> X(getLtumnfRISCV64Target());
}
