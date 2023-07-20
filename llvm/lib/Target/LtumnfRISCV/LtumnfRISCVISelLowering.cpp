#include "LtumnfRISCVISelLowering.h"
#include "MCTargetDesc/LtumnfRISCVMCTargetDesc.h"
#include "LtumnfRISCVSubtarget.h"
#include "LtumnfRISCVTargetMachine.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"

using namespace llvm;

#include "LtumnfRISCVGenCallingConv.inc"

LtumnfRISCVTargetLowering::LtumnfRISCVTargetLowering(
    const TargetMachine &TM, const LtumnfRISCVSubtarget &STI)
    : TargetLowering(TM), Subtarget(STI) {
  MVT XLenVT = Subtarget.getXLenVT();
  addRegisterClass(XLenVT, &LtumnfRISCV::GPRRegClass);

  computeRegisterProperties(Subtarget.getRegisterInfo());

  setStackPointerRegisterToSaveRestore(LtumnfRISCV::X2);

  for (auto N : {ISD::EXTLOAD, ISD::SEXTLOAD, ISD::ZEXTLOAD})
    setLoadExtAction(N, XLenVT, MVT::i1, Promote);

  // TODO: add all necessary setOperationAction calls.

  // Function alignments
  setMinFunctionAlignment(Align(4));
  setPrefFunctionAlignment(Align(4));
}

const char *
LtumnfRISCVTargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
  case LtumnfRISCVISD::CALL:
    return "LtumnfRISCVISD::CALL";
  case LtumnfRISCVISD::RET_FLAG:
    return "LtumnfRISCVISD::RET_FLAG";
  default:
    return nullptr;
  }
}

bool LtumnfRISCVTargetLowering::isSExtCheaperThanZExt(EVT SrcVT,
                                                      EVT DstVT) const {
  return SrcVT == MVT::i32 && DstVT == MVT::i64;
}

SDValue
LtumnfRISCVTargetLowering::LowerCall(TargetLowering::CallLoweringInfo &CLI,
                                     SmallVectorImpl<SDValue> &InVals) const {
  SelectionDAG &DAG = CLI.DAG;
  SDLoc &DL = CLI.DL;
  SmallVectorImpl<ISD::OutputArg> &Outs = CLI.Outs;
  SmallVectorImpl<SDValue> &OutVals = CLI.OutVals;
  SmallVectorImpl<ISD::InputArg> &Ins = CLI.Ins;
  SDValue Chain = CLI.Chain;
  SDValue Callee = CLI.Callee;
  bool &IsTailCall = CLI.IsTailCall;
  CallingConv::ID CallConv = CLI.CallConv;
  bool IsVarArg = CLI.IsVarArg;

  // not yet support tail call optimization.
  IsTailCall = false;

  if (CallConv != CallingConv::C)
    report_fatal_error("Unsupported calling convention");

  if (IsVarArg)
    report_fatal_error("VarArg not supported");

  // Analyze operands of the call, assigning locations to each operand.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState ArgCCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), ArgLocs,
                    *DAG.getContext());
  GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee);
  MachineFrameInfo &MFI = DAG.getMachineFunction().getFrameInfo();
  ArgCCInfo.AnalyzeCallOperands(Outs, CC_LtumnfRISCV);

  // Get a count of how many bytes are to be pushed on the stack.
  unsigned NumBytes = ArgCCInfo.getNextStackOffset();

  Chain = DAG.getCALLSEQ_START(Chain, NumBytes, 0, DL);

  SmallVector<std::pair<unsigned, SDValue>, 4> RegsToPass;
  SmallVector<SDValue, 12> MemOpChains;
  SDValue StackPtr;

  // Walk the register/memloc assignments, inserting copies/loads.
  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];
    SDValue Arg = OutVals[i];
    ISD::ArgFlagsTy Flags = Outs[i].Flags;

    // Promote the value if needed.
    switch (VA.getLocInfo()) {
    case CCValAssign::Full:
      break;
    // case CCValAssign::SExt:
    //   Arg = DAG.getNode(ISD::SIGN_EXTEND, DL, VA.getLocVT(), Arg);
    //   break;
    // case CCValAssign::ZExt:
    //   Arg = DAG.getNode(ISD::ZERO_EXTEND, DL, VA.getLocVT(), Arg);
    //   break;
    // case CCValAssign::AExt:
    //   Arg = DAG.getNode(ISD::ANY_EXTEND, DL, VA.getLocVT(), Arg);
    //   break;
    default:
      llvm_unreachable("Unknown loc info!");
    }

    // Arguments that can be passed on register must be kept at RegsToPass
    // vector
    if (VA.isRegLoc()) {
      RegsToPass.push_back(std::make_pair(VA.getLocReg(), Arg));
    } else {
      assert(VA.isMemLoc());

      if (StackPtr.getNode() == nullptr)
        StackPtr = DAG.getCopyFromReg(Chain, DL, LtumnfRISCV::X2,
                                      getPointerTy(DAG.getDataLayout()));

      SDValue PtrOff =
          DAG.getNode(ISD::ADD, DL, getPointerTy(DAG.getDataLayout()), StackPtr,
                      DAG.getIntPtrConstant(VA.getLocMemOffset(), DL));

      MemOpChains.push_back(
          DAG.getStore(Chain, DL, Arg, PtrOff, MachinePointerInfo()));
    }
  }

  // Transform all store nodes into one single node because all store nodes are
  // independent of each other.
  if (!MemOpChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, MemOpChains);

  // Build a sequence of copy-to-reg nodes chained together with token chain and
  // flag operands which copy the outgoing args into registers.  The InFlag in
  // necessary since all emitted instructions must be stuck together.
  SDValue InFlag;
  for (auto &Reg : RegsToPass) {
    Chain = DAG.getCopyToReg(Chain, DL, Reg.first, Reg.second, InFlag);
    InFlag = Chain.getValue(1);
  }

  // We only support calling global addresses.
  assert(G && "We only support the calling of global addresses");
  // FIXME: Target Operand Flag for getTargetGlobalAddress() ?
  Callee = DAG.getTargetGlobalAddress(G->getGlobal(), DL,
                                      getPointerTy(DAG.getDataLayout()), 0);

  // Returns a chain & a flag for retval copy to use.
  SmallVector<SDValue, 8> Ops;
  Ops.push_back(Chain);
  Ops.push_back(Callee);

  // Add argument registers to the end of the list so that they are
  // known live into the call.
  for (auto &Reg : RegsToPass)
    Ops.push_back(DAG.getRegister(Reg.first, Reg.second.getValueType()));

  // Add a register mask operand representing the call-preserved registers.
  const TargetRegisterInfo *TRI = Subtarget.getRegisterInfo();
  const uint32_t *Mask =
      TRI->getCallPreservedMask(DAG.getMachineFunction(), CallConv);
  assert(Mask && "Missing call preserved mask for calling convention");
  Ops.push_back(DAG.getRegisterMask(Mask));

  if (InFlag.getNode())
    Ops.push_back(InFlag);

  SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);
  Chain = DAG.getNode(LtumnfRISCVISD::CALL, DL, NodeTys, Ops);
  InFlag = Chain.getValue(1);

  // Mark the end of the call, which is glued to the call itself.
  Chain = DAG.getCALLSEQ_END(Chain, NumBytes, 0, InFlag, DL);
  InFlag = Chain.getValue(1);

  // Assign locations to each value returned by this call.
  SmallVector<CCValAssign, 16> RVLocs;
  CCState RetCCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), RVLocs,
                    *DAG.getContext());

  RetCCInfo.AnalyzeCallResult(Ins, RetCC_LtumnfRISCV);

  // Copy all of the result registers out of their specified physreg.
  for (auto &VA : RVLocs) {
    // Copy the value out
    SDValue RetValue =
        DAG.getCopyFromReg(Chain, DL, VA.getLocReg(), VA.getLocVT(), InFlag);
    // Glue the RetValue to the end of the call sequence
    Chain = RetValue.getValue(1);
    InFlag = RetValue.getValue(2);
    InVals.push_back(RetValue);
  }

  return Chain;
}

SDValue LtumnfRISCVTargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {

  if (CallConv != CallingConv::C)
    report_fatal_error("Unsupported calling convention");

  if (IsVarArg)
    report_fatal_error("VarArg not supported");

  MachineFunction &MF = DAG.getMachineFunction();
  MachineRegisterInfo &RegInfo = MF.getRegInfo();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MVT XLenVT = Subtarget.getXLenVT();

  // Assign locations to all of the incoming arguments.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeFormalArguments(Ins, CC_LtumnfRISCV);

  for (unsigned i = 0, e = ArgLocs.size(); i < e; ++i) {
    CCValAssign &VA = ArgLocs[i];
    EVT LocVT = VA.getLocVT();
    EVT ValVT = VA.getValVT();
    if (ValVT != XLenVT)
      report_fatal_error(
          "LowerFormalArguments only support MVT::i64 argument type");
    if (VA.isRegLoc()) {
      const TargetRegisterClass *RC = getRegClassFor(LocVT.getSimpleVT());
      Register VReg = RegInfo.createVirtualRegister(RC);
      RegInfo.addLiveIn(VA.getLocReg(), VReg);
      SDValue Val = DAG.getCopyFromReg(Chain, DL, VReg, LocVT);
      InVals.push_back(Val);
    } else /* isMemLoc() */ {
      EVT PtrVT =
          MVT::getIntegerVT(DAG.getDataLayout().getPointerSizeInBits(0));
      int FI = MFI.CreateFixedObject(ValVT.getStoreSize(), VA.getLocMemOffset(),
                                     /*IsImmutable=*/true);
      SDValue FIN = DAG.getFrameIndex(FI, PtrVT);
      SDValue Val = DAG.getExtLoad(
          ISD::NON_EXTLOAD, DL, LocVT, Chain, FIN,
          MachinePointerInfo::getFixedStack(DAG.getMachineFunction(), FI),
          ValVT);
      InVals.push_back(Val);
    }
  }

  return Chain;
}

SDValue LtumnfRISCVTargetLowering::LowerReturn(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs,
    const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
    SelectionDAG &DAG) const {
  if (IsVarArg) {
    report_fatal_error("VarArg not supported");
  }

  SmallVector<CCValAssign, 16> RVLocs;

  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), RVLocs,
                 *DAG.getContext());
  CCInfo.AnalyzeReturn(Outs, RetCC_LtumnfRISCV);

  SDValue Glue;
  SmallVector<SDValue, 4> RetOps(1, Chain);

  // Copy the result values into the output registers.
  for (unsigned i = 0, e = RVLocs.size(); i < e; ++i) {
    CCValAssign &VA = RVLocs[i];
    assert(VA.isRegLoc() && "Can only return in registers!");
    Chain = DAG.getCopyToReg(Chain, DL, VA.getLocReg(), OutVals[i], Glue);
    Glue = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
  }

  RetOps[0] = Chain;

  if (Glue.getNode()) {
    RetOps.push_back(Glue);
  }

  return DAG.getNode(LtumnfRISCVISD::RET_FLAG, DL, MVT::Other, RetOps);
}
