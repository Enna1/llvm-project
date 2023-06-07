#ifndef LLVM_LIB_TARGET_REMNIWRISCV_REMNIWRISCVTARGETMACHINE_H
#define LLVM_LIB_TARGET_REMNIWRISCV_REMNIWRISCVTARGETMACHINE_H

#include "RemniwRISCVSubtarget.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class RemniwRISCVTargetMachine : public LLVMTargetMachine {
  RemniwRISCVSubtarget Subtarget;
  std::unique_ptr<TargetLoweringObjectFile> TLOF;

public:
  RemniwRISCVTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                           StringRef FS, const TargetOptions &Options,
                           std::optional<Reloc::Model> RM,
                           std::optional<CodeModel::Model> CM,
                           CodeGenOpt::Level OL, bool JIT);

  const RemniwRISCVSubtarget *
  getSubtargetImpl(const Function &F) const override {
    return &Subtarget;
  }

  const RemniwRISCVSubtarget *getSubtargetImpl() const { return &Subtarget; }

  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;

  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_REMNIWRISCV_REMNIWRISCVTARGETMACHINE_H
