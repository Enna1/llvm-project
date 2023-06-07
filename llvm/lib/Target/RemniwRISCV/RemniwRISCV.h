#ifndef LLVM_LIB_TARGET_REMNIWRISCV_REMNIWRISCV_H
#define LLVM_LIB_TARGET_REMNIWRISCV_REMNIWRISCV_H

#include "llvm/Target/TargetMachine.h"

namespace llvm {

class RemniwRISCVTargetMachine;
class FunctionPass;

FunctionPass *createRemniwRISCVISelDag(RemniwRISCVTargetMachine &TM,
                                       CodeGenOpt::Level OptLevel);

} // end namespace llvm

#endif // LLVM_LIB_TARGET_REMNIWRISCV_REMNIWRISCV_H
