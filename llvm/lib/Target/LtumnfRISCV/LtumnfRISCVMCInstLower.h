#ifndef LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCVMCINSTLOWER_H
#define LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCVMCINSTLOWER_H

namespace llvm {

class MCContext;
class AsmPrinter;
class MachineInstr;
class MachineOperand;
class MCInst;
class MCOperand;

class LtumnfRISCVMCInstLower {
  MCContext &Ctx;
  AsmPrinter &Printer;

public:
  LtumnfRISCVMCInstLower(MCContext &Ctx, AsmPrinter &Printer);

  void lower(const MachineInstr *MI, MCInst &OutMI) const;

  bool lowerOperand(const MachineOperand &MO, MCOperand &MCOp) const;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCVMCINSTLOWER_H
