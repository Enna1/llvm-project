#include "RemniwRISCVMCInstLower.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"

using namespace llvm;

RemniwRISCVMCInstLower::RemniwRISCVMCInstLower(MCContext &Ctx,
                                               AsmPrinter &Printer)
    : Ctx(Ctx), Printer(Printer) {}

void RemniwRISCVMCInstLower::Lower(const MachineInstr *MI,
                                   MCInst &OutMI) const {
  OutMI.setOpcode(MI->getOpcode());

  for (const MachineOperand &MO : MI->operands()) {
    MCOperand MCOp;
    if (LowerOperand(MO, MCOp)) {
      OutMI.addOperand(MCOp);
    }
  }
}

bool RemniwRISCVMCInstLower::LowerOperand(const MachineOperand &MO,
                                          MCOperand &MCOp) const {
  switch (MO.getType()) {
  case MachineOperand::MO_Register:
    if (MO.isImplicit()) {
      return false;
    }
    MCOp = MCOperand::createReg(MO.getReg());
    break;
  case MachineOperand::MO_Immediate:
    MCOp = MCOperand::createImm(MO.getImm());
    break;
  default:
    llvm_unreachable("Unknown operand type!");
  }
  return true;
}
