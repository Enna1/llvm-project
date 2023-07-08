#include "MCTargetDesc/RemniwRISCVMCTargetDesc.h"
#include "RemniwRISCV.h"
#include "RemniwRISCVTargetMachine.h"
#include "llvm/CodeGen/SelectionDAGISel.h"

using namespace llvm;

namespace {

class RemniwRISCVDAGToDAGISel : public SelectionDAGISel {
  const RemniwRISCVSubtarget *Subtarget = nullptr;

public:
  static char ID;

  explicit RemniwRISCVDAGToDAGISel(RemniwRISCVTargetMachine &TM,
                                   CodeGenOpt::Level OptLevel)
      : SelectionDAGISel(ID, TM, OptLevel) {}

  StringRef getPassName() const override {
    return "RemniwRISCV DAG->DAG Pattern Instruction Selection";
  }

  bool runOnMachineFunction(MachineFunction &MF) override {
    Subtarget = &MF.getSubtarget<RemniwRISCVSubtarget>();
    return SelectionDAGISel::runOnMachineFunction(MF);
  }

  void Select(SDNode *N) override;

  bool SelectAddrFrameIndex(SDValue Addr, SDValue &Base, SDValue &Offset);
  bool SelectFrameAddrRegImm(SDValue Addr, SDValue &Base, SDValue &Offset);
  bool SelectAddrRegImm(SDValue Addr, SDValue &Base, SDValue &Offset);

private:
#include "RemniwRISCVGenDAGISel.inc"
};

} // end anonymous namespace

void RemniwRISCVDAGToDAGISel::Select(SDNode *N) {
  SDLoc DL(N);

  switch (N->getOpcode()) {
  case ISD::Constant: {
    auto *ConstNode = cast<ConstantSDNode>(N);
    int64_t Imm = ConstNode->getSExtValue();
    if (-2048 <= Imm && Imm <= 2047) {
      SDValue SDImm = CurDAG->getTargetConstant(Imm, DL, MVT::i64);
      SDValue SrcReg = CurDAG->getRegister(RemniwRISCV::X0, MVT::i64);
      SDNode *Result = CurDAG->getMachineNode(RemniwRISCV::ADDI, DL, MVT::i64,
                                              SrcReg, SDImm);
      ReplaceNode(N, Result);
      return;
    } else {
      report_fatal_error("unsupported Imm Range");
    }
  }
  }

  SelectCode(N);
}

bool RemniwRISCVDAGToDAGISel::SelectAddrFrameIndex(SDValue Addr, SDValue &Base,
                                                   SDValue &Offset) {
  if (auto *FIN = dyn_cast<FrameIndexSDNode>(Addr)) {
    Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), Subtarget->getXLenVT());
    Offset = CurDAG->getTargetConstant(0, SDLoc(Addr), Subtarget->getXLenVT());
    return true;
  }

  return false;
}

// Select a frame index and an optional immediate offset from an ADD or OR.
bool RemniwRISCVDAGToDAGISel::SelectFrameAddrRegImm(SDValue Addr, SDValue &Base,
                                                    SDValue &Offset) {
  if (SelectAddrFrameIndex(Addr, Base, Offset))
    return true;

  // TODO
  return false;
}

bool RemniwRISCVDAGToDAGISel::SelectAddrRegImm(SDValue Addr, SDValue &Base,
                                               SDValue &Offset) {
  if (SelectAddrFrameIndex(Addr, Base, Offset))
    return true;

  SDLoc DL(Addr);
  MVT VT = Addr.getSimpleValueType();

  Base = Addr;
  Offset = CurDAG->getTargetConstant(0, DL, VT);
  return true;
}

FunctionPass *llvm::createRemniwRISCVISelDag(RemniwRISCVTargetMachine &TM,
                                             CodeGenOpt::Level OptLevel) {
  return new RemniwRISCVDAGToDAGISel(TM, OptLevel);
}

char RemniwRISCVDAGToDAGISel::ID = 0;

// FIXME
// INITIALIZE_PASS(RemniwRISCVDAGToDAGISel, DEBUG_TYPE, PASS_NAME, false, false)
