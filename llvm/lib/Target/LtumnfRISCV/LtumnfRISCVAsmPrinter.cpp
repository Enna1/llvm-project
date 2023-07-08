#include "MCTargetDesc/LtumnfRISCVMCTargetDesc.h"
#include "LtumnfRISCVMCInstLower.h"
#include "TargetInfo/LtumnfRISCVTargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

namespace {

class LtumnfRISCVAsmPrinter : public AsmPrinter {
  LtumnfRISCVMCInstLower MCInstLowering;

public:
  LtumnfRISCVAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
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

#include "LtumnfRISCVGenMCPseudoLowering.inc"

void LtumnfRISCVAsmPrinter::emitInstruction(const MachineInstr *MI) {
  if (emitPseudoExpansionLowering(*OutStreamer, MI)) {
    return;
  }

  MCInst TmpInst;
  MCInstLowering.lower(MI, TmpInst);
  EmitToStreamer(*OutStreamer, TmpInst);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeLtumnfRISCVAsmPrinter() {
  RegisterAsmPrinter<LtumnfRISCVAsmPrinter> X(getLtumnfRISCV64Target());
}
