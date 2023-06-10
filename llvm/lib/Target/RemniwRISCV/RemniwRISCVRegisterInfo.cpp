#include "RemniwRISCVRegisterInfo.h"
#include "MCTargetDesc/RemniwRISCVMCTargetDesc.h"
#include "RemniwRISCVFrameLowering.h"
#include "RemniwRISCVSubtarget.h"
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
  return CC_Save_SaveList;
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
    MachineBasicBlock::iterator MI, int SPAdj, unsigned FIOperandNum,
    RegScavenger *RS) const {
  return false;
}

Register
RemniwRISCVRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return RemniwRISCV::X8;
}
