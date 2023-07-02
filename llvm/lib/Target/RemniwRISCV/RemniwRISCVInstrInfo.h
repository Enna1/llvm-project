#ifndef LLVM_LIB_TARGET_REMNIWRISCV_REMNIWRISCVINSTRINFO_H
#define LLVM_LIB_TARGET_REMNIWRISCV_REMNIWRISCVINSTRINFO_H

#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "RemniwRISCVGenInstrInfo.inc"

namespace llvm {

class RemniwRISCVSubtarget;

class RemniwRISCVInstrInfo : public RemniwRISCVGenInstrInfo {
  const RemniwRISCVSubtarget &STI;

public:
  explicit RemniwRISCVInstrInfo(RemniwRISCVSubtarget &STI);

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
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_REMNIWRISCV_REMNIWRISCVINSTRINFO_H
