#include "TargetInfo/RemniwRISCVTargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

namespace llvm {

Target &getRemniwRISCV64Target() {
  static Target RemniwRISCV64Target;
  return RemniwRISCV64Target;
}

} // end namespace llvm

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRemniwRISCVTargetInfo() {
  RegisterTarget<Triple::remniwriscv64> X(
      getRemniwRISCV64Target(), "remniwriscv64",
      "RemniwRISC-V (64-bit): remniwriscv64", "RISCV");
}
