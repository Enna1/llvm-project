#ifndef LLVM_LIB_TARGET_REMNIWRISCV_REMNIWRISCVMCINSTLOWER_H
#define LLVM_LIB_TARGET_REMNIWRISCV_REMNIWRISCVMCINSTLOWER_H

namespace llvm {

class MCContext;
class AsmPrinter;
class MachineInstr;
class MachineOperand;
class MCInst;
class MCOperand;

class RemniwRISCVMCInstLower {
  MCContext &Ctx;
  AsmPrinter &Printer;

public:
  RemniwRISCVMCInstLower(MCContext &Ctx, AsmPrinter &Printer);

  void Lower(const MachineInstr *MI, MCInst &OutMI) const;

private:
  bool LowerOperand(const MachineOperand &MO, MCOperand &MCOp) const;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_REMNIWRISCV_REMNIWRISCVMCINSTLOWER_H
