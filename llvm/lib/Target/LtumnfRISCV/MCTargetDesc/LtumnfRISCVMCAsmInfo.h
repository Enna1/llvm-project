#ifndef LLVM_LIB_TARGET_LTUMNFRISCV_MCTARGETDESC_LTUMNFRISCVMCASMINFO_H
#define LLVM_LIB_TARGET_LTUMNFRISCV_MCTARGETDESC_LTUMNFRISCVMCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {

class Triple;

class LtumnfRISCVMCAsmInfo : public MCAsmInfoELF {
public:
  explicit LtumnfRISCVMCAsmInfo(const Triple &TT);
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_LTUMNFRISCV_MCTARGETDESC_LTUMNFRISCVMCASMINFO_H
