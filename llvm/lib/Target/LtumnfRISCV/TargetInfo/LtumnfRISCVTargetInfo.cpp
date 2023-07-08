#include "TargetInfo/LtumnfRISCVTargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

namespace llvm {

Target &getLtumnfRISCV64Target() {
  static Target LtumnfRISCV64Target;
  return LtumnfRISCV64Target;
}

} // end namespace llvm

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeLtumnfRISCVTargetInfo() {
  RegisterTarget<Triple::ltumnfriscv64> X(
      getLtumnfRISCV64Target(), "ltumnfriscv64",
      "LtumnfRISC-V (64-bit): ltumnfriscv64", "RISCV");
}
