#include "RemniwRISCVInstrInfo.h"
#include "MCTargetDesc/RemniwRISCVMCTargetDesc.h"
#include "RemniwRISCVSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "RemniwRISCVGenInstrInfo.inc"

RemniwRISCVInstrInfo::RemniwRISCVInstrInfo(RemniwRISCVSubtarget &STI)
    : RemniwRISCVGenInstrInfo(), STI(STI) {}

void RemniwRISCVInstrInfo::storeRegToStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator I, Register SrcReg,
    bool IsKill, int FI, const TargetRegisterClass *RC,
    const TargetRegisterInfo *TRI, Register VReg) const {
  DebugLoc DL;
  if (I != MBB.end())
    DL = I->getDebugLoc();

  MachineFunction *MF = MBB.getParent();
  MachineFrameInfo &MFI = MF->getFrameInfo();

  MachineMemOperand *MMO = MF->getMachineMemOperand(
      MachinePointerInfo::getFixedStack(*MF, FI), MachineMemOperand::MOStore,
      MFI.getObjectSize(FI), MFI.getObjectAlign(FI));

  BuildMI(MBB, I, DL, get(RemniwRISCV::SD))
      .addReg(SrcReg, getKillRegState(IsKill))
      .addFrameIndex(FI)
      .addImm(0)
      .addMemOperand(MMO);
}

void RemniwRISCVInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                                MachineBasicBlock::iterator I,
                                                Register DstReg, int FI,
                                                const TargetRegisterClass *RC,
                                                const TargetRegisterInfo *TRI,
                                                Register VReg) const {
  DebugLoc DL;
  if (I != MBB.end())
    DL = I->getDebugLoc();

  MachineFunction *MF = MBB.getParent();
  MachineFrameInfo &MFI = MF->getFrameInfo();

  MachineMemOperand *MMO = MF->getMachineMemOperand(
      MachinePointerInfo::getFixedStack(*MF, FI), MachineMemOperand::MOLoad,
      MFI.getObjectSize(FI), MFI.getObjectAlign(FI));

  BuildMI(MBB, I, DL, get(RemniwRISCV::LD), DstReg)
      .addFrameIndex(FI)
      .addImm(0)
      .addMemOperand(MMO);
}
