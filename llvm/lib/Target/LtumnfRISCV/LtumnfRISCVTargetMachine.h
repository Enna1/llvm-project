#ifndef LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCVTARGETMACHINE_H
#define LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCVTARGETMACHINE_H

#include "LtumnfRISCVSubtarget.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class LtumnfRISCVTargetMachine : public LLVMTargetMachine {
  LtumnfRISCVSubtarget Subtarget;
  std::unique_ptr<TargetLoweringObjectFile> TLOF;

public:
  LtumnfRISCVTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                           StringRef FS, const TargetOptions &Options,
                           std::optional<Reloc::Model> RM,
                           std::optional<CodeModel::Model> CM,
                           CodeGenOpt::Level OL, bool JIT);

  const LtumnfRISCVSubtarget *
  getSubtargetImpl(const Function &F) const override {
    return &Subtarget;
  }

  const LtumnfRISCVSubtarget *getSubtargetImpl() const { return &Subtarget; }

  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;

  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCVTARGETMACHINE_H
