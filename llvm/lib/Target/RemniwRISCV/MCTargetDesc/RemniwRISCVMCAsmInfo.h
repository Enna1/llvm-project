#ifndef LLVM_LIB_TARGET_REMNIWRISCV_MCTARGETDESC_REMNIWRISCVMCASMINFO_H
#define LLVM_LIB_TARGET_REMNIWRISCV_MCTARGETDESC_REMNIWRISCVMCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {

class Triple;

class RemniwRISCVMCAsmInfo : public MCAsmInfoELF {
public:
  explicit RemniwRISCVMCAsmInfo(const Triple &TT);
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_REMNIWRISCV_MCTARGETDESC_REMNIWRISCVMCASMINFO_H
