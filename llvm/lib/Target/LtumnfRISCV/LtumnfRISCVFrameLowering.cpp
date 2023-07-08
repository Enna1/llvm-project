#include "LtumnfRISCVFrameLowering.h"
#include "MCTargetDesc/LtumnfRISCVMCTargetDesc.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"

using namespace llvm;

void LtumnfRISCVFrameLowering::emitPrologue(MachineFunction &MF,
                                            MachineBasicBlock &MBB) const {
  // Compute the stack size, to determine if we need a prologue at all.
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  MachineBasicBlock::iterator MBBI = MBB.begin();
  DebugLoc DL;

  const MachineFrameInfo &MFI = MF.getFrameInfo();
  uint64_t StackSize = alignTo(MFI.getStackSize(), getStackAlign());
  Register SPReg = LtumnfRISCV::X2;
  // FIXME: handle StackSize out of range simm12, refactor with adjustReg()
  BuildMI(MBB, MBBI, DL, TII.get(LtumnfRISCV::ADDI), SPReg)
      .addReg(SPReg)
      .addImm(-StackSize)
      .setMIFlag(MachineInstr::FrameSetup);
}

void LtumnfRISCVFrameLowering::emitEpilogue(MachineFunction &MF,
                                            MachineBasicBlock &MBB) const {
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  // Get the insert location for the epilogue. If there were no terminators in
  // the block, get the last instruction.
  MachineBasicBlock::iterator MBBI = MBB.end();
  DebugLoc DL;
  if (!MBB.empty())
    MBBI = MBB.getFirstTerminator();

  const MachineFrameInfo &MFI = MF.getFrameInfo();
  uint64_t StackSize = alignTo(MFI.getStackSize(), getStackAlign());
  if (StackSize == 0)
    return;

  // FIXME: handle StackSize out of range simm12, refactor with adjustReg()
  Register SPReg = LtumnfRISCV::X2;
  BuildMI(MBB, MBBI, DL, TII.get(LtumnfRISCV::ADDI), SPReg)
      .addReg(SPReg)
      .addImm(StackSize)
      .setMIFlag(MachineInstr::FrameDestroy);
}

// Disable frame pointer elimination by default
bool LtumnfRISCVFrameLowering::hasFP(const MachineFunction &MF) const {
  return true;
}
