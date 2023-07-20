#ifndef LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCVINSTRINFO_H
#define LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCVINSTRINFO_H

#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "LtumnfRISCVGenInstrInfo.inc"

namespace llvm {

class LtumnfRISCVSubtarget;

class LtumnfRISCVInstrInfo : public LtumnfRISCVGenInstrInfo {
  const LtumnfRISCVSubtarget &STI;

public:
  explicit LtumnfRISCVInstrInfo(LtumnfRISCVSubtarget &STI);

  void storeRegToStackSlot(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MBBI, Register SrcReg,
                           bool IsKill, int FrameIndex,
                           const TargetRegisterClass *RC,
                           const TargetRegisterInfo *TRI,
                           Register VReg) const override;

  void loadRegFromStackSlot(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MBBI, Register DstReg,
                            int FrameIndex, const TargetRegisterClass *RC,
                            const TargetRegisterInfo *TRI,
                            Register VReg) const override;

  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                   const DebugLoc &DL, MCRegister DstReg, MCRegister SrcReg,
                   bool KillSrc) const override;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCVINSTRINFO_H
