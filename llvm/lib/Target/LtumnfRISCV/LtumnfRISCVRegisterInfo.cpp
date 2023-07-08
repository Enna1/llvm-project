#include "LtumnfRISCVRegisterInfo.h"
#include "MCTargetDesc/LtumnfRISCVMCTargetDesc.h"
#include "LtumnfRISCVFrameLowering.h"
#include "LtumnfRISCVSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"

#define GET_REGINFO_TARGET_DESC
#include "LtumnfRISCVGenRegisterInfo.inc"

using namespace llvm;

LtumnfRISCVRegisterInfo::LtumnfRISCVRegisterInfo(unsigned HwMode)
    : LtumnfRISCVGenRegisterInfo(LtumnfRISCV::X1, /*DwarfFlavour*/ 0,
                                 /*EHFlavor*/ 0,
                                 /*PC*/ 0, HwMode) {}

const MCPhysReg *
LtumnfRISCVRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return CSR_SaveList;
}

BitVector
LtumnfRISCVRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  Reserved.set(LtumnfRISCV::X0); // zero
  Reserved.set(LtumnfRISCV::X2); // sp
  Reserved.set(LtumnfRISCV::X3); // gp
  Reserved.set(LtumnfRISCV::X4); // tp
  Reserved.set(LtumnfRISCV::X8); // fp
  return Reserved;
}

bool LtumnfRISCVRegisterInfo::eliminateFrameIndex(
    MachineBasicBlock::iterator II, int SPAdj, unsigned FIOperandNum,
    RegScavenger *RS) const {
  assert(SPAdj == 0 && "Unexpected non-zero SPAdj value");

  MachineInstr &MI = *II;
  MachineFunction &MF = *MI.getParent()->getParent();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const LtumnfRISCVSubtarget &ST = MF.getSubtarget<LtumnfRISCVSubtarget>();
  DebugLoc DL = MI.getDebugLoc();

  int FrameIndex = MI.getOperand(FIOperandNum).getIndex();
  Register FrameReg = LtumnfRISCV::X2;
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
LtumnfRISCVRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return LtumnfRISCV::X8;
}

const uint32_t *
LtumnfRISCVRegisterInfo::getCallPreservedMask(const MachineFunction & /*MF*/,
                                              CallingConv::ID /*CC*/) const {

  // CSR defined in LtumnfRISCVCallingConv.td, CSR_RegMask defined in
  // LtumnfRISCVGenRegisterInfo.inc
  return CSR_RegMask;
}
