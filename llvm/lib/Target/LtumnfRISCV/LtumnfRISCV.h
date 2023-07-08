#ifndef LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCV_H
#define LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCV_H

#include "llvm/Target/TargetMachine.h"

namespace llvm {

class LtumnfRISCVTargetMachine;
class FunctionPass;

FunctionPass *createLtumnfRISCVISelDag(LtumnfRISCVTargetMachine &TM,
                                       CodeGenOpt::Level OptLevel);

} // end namespace llvm

#endif // LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCV_H
