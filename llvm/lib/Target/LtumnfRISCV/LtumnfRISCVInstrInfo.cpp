#include "LtumnfRISCVInstrInfo.h"
#include "MCTargetDesc/LtumnfRISCVMCTargetDesc.h"
#include "LtumnfRISCVSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "LtumnfRISCVGenInstrInfo.inc"

LtumnfRISCVInstrInfo::LtumnfRISCVInstrInfo(LtumnfRISCVSubtarget &STI)
    : LtumnfRISCVGenInstrInfo(), STI(STI) {}

void LtumnfRISCVInstrInfo::storeRegToStackSlot(
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

  BuildMI(MBB, I, DL, get(LtumnfRISCV::SD))
      .addReg(SrcReg, getKillRegState(IsKill))
      .addFrameIndex(FI)
      .addImm(0)
      .addMemOperand(MMO);
}

void LtumnfRISCVInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
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

  BuildMI(MBB, I, DL, get(LtumnfRISCV::LD), DstReg)
      .addFrameIndex(FI)
      .addImm(0)
      .addMemOperand(MMO);
}

void LtumnfRISCVInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MBBI,
                                 const DebugLoc &DL, MCRegister DstReg,
                                 MCRegister SrcReg, bool KillSrc) const {
  if (!LtumnfRISCV::GPRRegClass.contains(DstReg, SrcReg)) {
    llvm_unreachable("Impossible reg-to-reg copy");
  }

  BuildMI(MBB, MBBI, DL, get(LtumnfRISCV::ADDI), DstReg)
      .addReg(SrcReg, getKillRegState(KillSrc))
      .addImm(0);
}
