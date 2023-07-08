#include "LtumnfRISCVMCAsmInfo.h"
#include "llvm/ADT/Triple.h"

using namespace llvm;

LtumnfRISCVMCAsmInfo::LtumnfRISCVMCAsmInfo(const Triple &TT) {
  assert(TT.isArch64Bit() && "Only RV64 is currently supported!");
  CodePointerSize = CalleeSaveStackSlotSize = TT.isArch64Bit() ? 8 : 4;
  CommentString = "#";
  Data16bitsDirective = "\t.half\t";
  Data32bitsDirective = "\t.word\t";
}
