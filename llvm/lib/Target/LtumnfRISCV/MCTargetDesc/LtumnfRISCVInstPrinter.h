#ifndef LLVM_LIB_TARGET_LTUMNFRISCV_MCTARGETDESC_LTUMNFRISCVINSTPRINTER_H
#define LLVM_LIB_TARGET_LTUMNFRISCV_MCTARGETDESC_LTUMNFRISCVINSTPRINTER_H

#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCRegister.h"

namespace llvm {

class LtumnfRISCVInstPrinter : public MCInstPrinter {
public:
  LtumnfRISCVInstPrinter(const MCAsmInfo &MAI, const MCInstrInfo &MII,
                         const MCRegisterInfo &MRI)
      : MCInstPrinter(MAI, MII, MRI) {}

  void printInst(const MCInst *MI, uint64_t Address, StringRef Annot,
                 const MCSubtargetInfo &STI, raw_ostream &OS) override;

private:
  std::pair<const char *, uint64_t> getMnemonic(const MCInst *MI) override;

  void printInstruction(const MCInst *MI, uint64_t Address, raw_ostream &O);

  bool printAliasInstr(const MCInst *MI, uint64_t Address, raw_ostream &O);

  static const char *getRegisterName(MCRegister Reg, unsigned AltIdx);

  void printCustomAliasOperand(const MCInst *MI, uint64_t Address,
                               unsigned OpIdx, unsigned PrintMethodIdx,
                               raw_ostream &O);

  void printOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O);
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_LTUMNFRISCV_MCTARGETDESC_LTUMNFRISCVINSTPRINTER_H
