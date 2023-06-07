#ifndef LLVM_LIB_TARGET_REMNIWRISCV_REMNIWRISCVFRAMELOWERING_H
#define LLVM_LIB_TARGET_REMNIWRISCV_REMNIWRISCVFRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {

class RemniwRISCVSubtarget;

class RemniwRISCVFrameLowering : public TargetFrameLowering {
  const RemniwRISCVSubtarget &STI;

public:
  explicit RemniwRISCVFrameLowering(const RemniwRISCVSubtarget &STI)
      : TargetFrameLowering(StackGrowsDown,
                            /*StackAlignment=*/Align(16),
                            /*LocalAreaOffset=*/0),
        STI(STI) {}

  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

  bool hasFP(const MachineFunction &MF) const override;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_REMNIWRISCV_REMNIWRISCVFRAMELOWERING_H
