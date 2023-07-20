#include "MCTargetDesc/LtumnfRISCVMCTargetDesc.h"
#include "LtumnfRISCV.h"
#include "LtumnfRISCVTargetMachine.h"
#include "llvm/CodeGen/SelectionDAGISel.h"

using namespace llvm;

namespace {

class LtumnfRISCVDAGToDAGISel : public SelectionDAGISel {
  const LtumnfRISCVSubtarget *Subtarget = nullptr;

public:
  static char ID;

  explicit LtumnfRISCVDAGToDAGISel(LtumnfRISCVTargetMachine &TM,
                                   CodeGenOpt::Level OptLevel)
      : SelectionDAGISel(ID, TM, OptLevel) {}

  StringRef getPassName() const override {
    return "LtumnfRISCV DAG->DAG Pattern Instruction Selection";
  }

  bool runOnMachineFunction(MachineFunction &MF) override {
    Subtarget = &MF.getSubtarget<LtumnfRISCVSubtarget>();
    return SelectionDAGISel::runOnMachineFunction(MF);
  }

  void Select(SDNode *N) override;

  bool SelectAddrFrameIndex(SDValue Addr, SDValue &Base, SDValue &Offset);
  bool SelectFrameAddrRegImm(SDValue Addr, SDValue &Base, SDValue &Offset);
  bool SelectAddrRegImm(SDValue Addr, SDValue &Base, SDValue &Offset);

private:
#include "LtumnfRISCVGenDAGISel.inc"
};

} // end anonymous namespace

void LtumnfRISCVDAGToDAGISel::Select(SDNode *N) {
  SDLoc DL(N);

  switch (N->getOpcode()) {
    // materialize contants in LtumnfRISCVInstInfo.td
    // case ISD::Constant: {
    //   auto *ConstNode = cast<ConstantSDNode>(N);
    //   int64_t Imm = ConstNode->getSExtValue();
    //   if (-2048 <= Imm && Imm <= 2047) {
    //     SDValue SDImm = CurDAG->getTargetConstant(Imm, DL, MVT::i64);
    //     SDValue SrcReg = CurDAG->getRegister(LtumnfRISCV::X0, MVT::i64);
    //     SDNode *Result = CurDAG->getMachineNode(LtumnfRISCV::ADDI, DL,
    //     MVT::i64,
    //                                             SrcReg, SDImm);
    //     ReplaceNode(N, Result);
    //     return;
    //   } else {
    //     report_fatal_error("unsupported Imm Range");
    //   }
    // }
  }

  SelectCode(N);
}

bool LtumnfRISCVDAGToDAGISel::SelectAddrFrameIndex(SDValue Addr, SDValue &Base,
                                                   SDValue &Offset) {
  if (auto *FIN = dyn_cast<FrameIndexSDNode>(Addr)) {
    Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), Subtarget->getXLenVT());
    Offset = CurDAG->getTargetConstant(0, SDLoc(Addr), Subtarget->getXLenVT());
    return true;
  }

  return false;
}

// Select a frame index and an optional immediate offset from an ADD or OR.
bool LtumnfRISCVDAGToDAGISel::SelectFrameAddrRegImm(SDValue Addr, SDValue &Base,
                                                    SDValue &Offset) {
  if (SelectAddrFrameIndex(Addr, Base, Offset))
    return true;

  // TODO
  return false;
}

bool LtumnfRISCVDAGToDAGISel::SelectAddrRegImm(SDValue Addr, SDValue &Base,
                                               SDValue &Offset) {
  if (SelectAddrFrameIndex(Addr, Base, Offset))
    return true;

  SDLoc DL(Addr);
  MVT VT = Addr.getSimpleValueType();

  if (isa<ConstantSDNode>(Addr)) {
    int64_t CVal = cast<ConstantSDNode>(Addr)->getSExtValue();
    int64_t Lo12 = SignExtend64<12>(CVal);
    int64_t Hi = (uint64_t)CVal - (uint64_t)Lo12;
    if (isInt<32>(Hi)) {
      if (Hi) {
        int64_t Hi20 = (Hi >> 12) & 0xfffff;
        Base = SDValue(
            CurDAG->getMachineNode(LtumnfRISCV::LUI, DL, VT,
                                  CurDAG->getTargetConstant(Hi20, DL, VT)),
            0);
      } else {
        Base = CurDAG->getRegister(LtumnfRISCV::X0, VT);
      }
      Offset = CurDAG->getTargetConstant(Lo12, DL, VT);
      return true;
    }
  }

  Base = Addr;
  Offset = CurDAG->getTargetConstant(0, DL, VT);
  return true;
}

FunctionPass *llvm::createLtumnfRISCVISelDag(LtumnfRISCVTargetMachine &TM,
                                             CodeGenOpt::Level OptLevel) {
  return new LtumnfRISCVDAGToDAGISel(TM, OptLevel);
}

char LtumnfRISCVDAGToDAGISel::ID = 0;

// FIXME
// INITIALIZE_PASS(LtumnfRISCVDAGToDAGISel, DEBUG_TYPE, PASS_NAME, false, false)
