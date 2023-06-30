#include "MCTargetDesc/RemniwRISCVMCTargetDesc.h"
#include "RemniwRISCVMCInstLower.h"
#include "TargetInfo/RemniwRISCVTargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

namespace {

class RemniwRISCVAsmPrinter : public AsmPrinter {
  RemniwRISCVMCInstLower MCInstLowering;

public:
  RemniwRISCVAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)), MCInstLowering(OutContext, *this) {
  }

  void emitInstruction(const MachineInstr *MI) override;

  // Wrapper needed for tblgenned pseudo lowering.
  bool lowerOperand(const MachineOperand &MO, MCOperand &MCOp) const {
    return MCInstLowering.lowerOperand(MO, MCOp);
  }

private:
  bool emitPseudoExpansionLowering(MCStreamer &OutStreamer,
                                   const MachineInstr *MI);
};

} // end anonymous namespace

#include "RemniwRISCVGenMCPseudoLowering.inc"

void RemniwRISCVAsmPrinter::emitInstruction(const MachineInstr *MI) {
  if (emitPseudoExpansionLowering(*OutStreamer, MI)) {
    return;
  }

  MCInst TmpInst;
  MCInstLowering.lower(MI, TmpInst);
  EmitToStreamer(*OutStreamer, TmpInst);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRemniwRISCVAsmPrinter() {
  RegisterAsmPrinter<RemniwRISCVAsmPrinter> X(getRemniwRISCV64Target());
}
