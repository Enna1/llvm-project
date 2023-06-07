#include "MCTargetDesc/RemniwRISCVMCTargetDesc.h"
#include "RemniwRISCV.h"
#include "RemniwRISCVTargetMachine.h"
#include "llvm/CodeGen/SelectionDAGISel.h"

using namespace llvm;

namespace {

class RemniwRISCVDAGToDAGISel : public SelectionDAGISel {
public:
  static char ID;

  explicit RemniwRISCVDAGToDAGISel(RemniwRISCVTargetMachine &TM,
                                   CodeGenOpt::Level OptLevel)
      : SelectionDAGISel(ID, TM, OptLevel) {}

  StringRef getPassName() const override {
    return "RemniwRISCV DAG->DAG Pattern Instruction Selection";
  }

  void Select(SDNode *N) override;

private:
#include "RemniwRISCVGenDAGISel.inc"
};

} // end anonymous namespace

void RemniwRISCVDAGToDAGISel::Select(SDNode *N) {
  SDLoc DL(N);

  switch (N->getOpcode()) {
  case ISD::Constant: {
    int64_t Imm = cast<ConstantSDNode>(N)->getSExtValue();
    if (-2048 <= Imm && Imm <= 2047) {
      SDValue SDImm = CurDAG->getTargetConstant(Imm, DL, MVT::i32);
      SDValue SrcReg = CurDAG->getRegister(RemniwRISCV::X0, MVT::i32);
      SDNode *Result = CurDAG->getMachineNode(RemniwRISCV::ADDI, DL, MVT::i32,
                                              SrcReg, SDImm);
      ReplaceNode(N, Result);
      return;
    }
  }
  }

  SelectCode(N);
}

FunctionPass *llvm::createRemniwRISCVISelDag(RemniwRISCVTargetMachine &TM,
                                             CodeGenOpt::Level OptLevel) {
  return new RemniwRISCVDAGToDAGISel(TM, OptLevel);
}

char RemniwRISCVDAGToDAGISel::ID = 0;

// FIXME
// INITIALIZE_PASS(RemniwRISCVDAGToDAGISel, DEBUG_TYPE, PASS_NAME, false, false)
