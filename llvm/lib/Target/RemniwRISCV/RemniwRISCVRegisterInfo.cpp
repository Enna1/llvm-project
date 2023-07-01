#include "RemniwRISCVRegisterInfo.h"
#include "MCTargetDesc/RemniwRISCVMCTargetDesc.h"
#include "RemniwRISCVFrameLowering.h"
#include "RemniwRISCVSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"

#define GET_REGINFO_TARGET_DESC
#include "RemniwRISCVGenRegisterInfo.inc"

using namespace llvm;

RemniwRISCVRegisterInfo::RemniwRISCVRegisterInfo(unsigned HwMode)
    : RemniwRISCVGenRegisterInfo(RemniwRISCV::X1, /*DwarfFlavour*/ 0,
                                 /*EHFlavor*/ 0,
                                 /*PC*/ 0, HwMode) {}

const MCPhysReg *
RemniwRISCVRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return CSR_SaveList;
}

BitVector
RemniwRISCVRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  Reserved.set(RemniwRISCV::X0); // zero
  Reserved.set(RemniwRISCV::X2); // sp
  Reserved.set(RemniwRISCV::X3); // gp
  Reserved.set(RemniwRISCV::X4); // tp
  Reserved.set(RemniwRISCV::X8); // fp
  return Reserved;
}

bool RemniwRISCVRegisterInfo::eliminateFrameIndex(
    MachineBasicBlock::iterator II, int SPAdj, unsigned FIOperandNum,
    RegScavenger *RS) const {
  assert(SPAdj == 0 && "Unexpected non-zero SPAdj value");

  MachineInstr &MI = *II;
  MachineFunction &MF = *MI.getParent()->getParent();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const RemniwRISCVSubtarget &ST = MF.getSubtarget<RemniwRISCVSubtarget>();
  DebugLoc DL = MI.getDebugLoc();

  int FrameIndex = MI.getOperand(FIOperandNum).getIndex();
  Register FrameReg = RemniwRISCV::X2;
  int64_t Offset = MFI.getObjectOffset(FrameIndex) + MFI.getStackSize() +
                   MI.getOperand(FIOperandNum + 1).getImm();

  // FIXME: check the range of offset.
  MI.getOperand(FIOperandNum)
      .ChangeToRegister(FrameReg, /*IsDef*/ false,
                        /*IsImp*/ false,
                        /*IsKill*/ false);
  MI.getOperand(FIOperandNum + 1).ChangeToImmediate(Offset);
  return true;
}

Register
RemniwRISCVRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return RemniwRISCV::X8;
}

const uint32_t *
RemniwRISCVRegisterInfo::getCallPreservedMask(const MachineFunction & /*MF*/,
                                              CallingConv::ID /*CC*/) const {

  // CSR defined in RemniwRISCVCallingConv.td, CSR_RegMask defined in
  // RemniwRISCVGenRegisterInfo.inc
  return CSR_RegMask;
}
