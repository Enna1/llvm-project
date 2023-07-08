#ifndef LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCVISELLOWERING_H
#define LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCVISELLOWERING_H

#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {

class LtumnfRISCVSubtarget;
class LtumnfRISCVTargetMachine;

namespace LtumnfRISCVISD {

enum NodeType {
  FIRST_NUMBER = ISD::BUILTIN_OP_END,
  // Return with a flag operand. Operand 0 is the chain operand.
  RET_FLAG,
  // CALL - These operations represent an abstract call instruction, which
  // includes a bunch of information.
  CALL,
};

} // namespace LtumnfRISCVISD

class LtumnfRISCVTargetLowering : public TargetLowering {
  const LtumnfRISCVSubtarget &Subtarget;

public:
  explicit LtumnfRISCVTargetLowering(const TargetMachine &TM,
                                     const LtumnfRISCVSubtarget &STI);

  // Provide custom lowering hooks for some operations.
  // SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;

  const char *getTargetNodeName(unsigned Opcode) const override;

private:
  SDValue LowerCall(TargetLowering::CallLoweringInfo &CLI,
                    SmallVectorImpl<SDValue> &InVals) const override;

  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
                               bool IsVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               const SDLoc &DL, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const override;

  SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
                      SelectionDAG &DAG) const override;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_LTUMNFRISCV_LTUMNFRISCVISELLOWERING_H
