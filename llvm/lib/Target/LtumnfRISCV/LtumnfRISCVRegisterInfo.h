#ifndef LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCVREGISTERINFO_H
#define LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCVREGISTERINFO_H

#include "llvm/CodeGen/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "LtumnfRISCVGenRegisterInfo.inc"

namespace llvm {

class LtumnfRISCVRegisterInfo : public LtumnfRISCVGenRegisterInfo {
public:
  explicit LtumnfRISCVRegisterInfo(unsigned HwMode);

  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF) const override;

  BitVector getReservedRegs(const MachineFunction &MF) const override;

  bool eliminateFrameIndex(MachineBasicBlock::iterator MI, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;

  Register getFrameRegister(const MachineFunction &MF) const override;

  const uint32_t *getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID) const override;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCVREGISTERINFO_H
