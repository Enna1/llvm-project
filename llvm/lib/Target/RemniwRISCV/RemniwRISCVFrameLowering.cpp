#include "RemniwRISCVFrameLowering.h"
#include "llvm/CodeGen/MachineFunction.h"

using namespace llvm;

void RemniwRISCVFrameLowering::emitPrologue(MachineFunction &MF,
                                            MachineBasicBlock &MBB) const {}

void RemniwRISCVFrameLowering::emitEpilogue(MachineFunction &MF,
                                            MachineBasicBlock &MBB) const {}

// Disable frame pointer elimination by default
bool RemniwRISCVFrameLowering::hasFP(const MachineFunction &MF) const {
  return true;
}
