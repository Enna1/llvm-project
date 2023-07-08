#ifndef LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCVFRAMELOWERING_H
#define LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCVFRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {

class LtumnfRISCVSubtarget;

class LtumnfRISCVFrameLowering : public TargetFrameLowering {
  const LtumnfRISCVSubtarget &STI;

public:
  explicit LtumnfRISCVFrameLowering(const LtumnfRISCVSubtarget &STI)
      : TargetFrameLowering(StackGrowsDown,
                            /*StackAlignment=*/Align(16),
                            /*LocalAreaOffset=*/0),
        STI(STI) {}

  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

  bool hasFP(const MachineFunction &MF) const override;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCVFRAMELOWERING_H
