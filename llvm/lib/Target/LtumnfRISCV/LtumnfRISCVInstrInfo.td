// This file describes the LtumnfRISCV instructions in TableGen format.

//===----------------------------------------------------------------------===//
// RISC-V specific DAG Nodes.
//===----------------------------------------------------------------------===//

// Target-independent type requirements, but with target-specific formats.
def SDT_CallSeqStart : SDCallSeqStart<[SDTCisVT<0, i64>,
                                       SDTCisVT<1, i64>]>;
def SDT_CallSeqEnd   : SDCallSeqEnd<[SDTCisVT<0, i64>,
                                     SDTCisVT<1, i64>]>;

// Target-dependent type requirements.
def SDT_LtumnfRISCVCall : SDTypeProfile<0, -1, [SDTCisVT<0, XLenVT>]>;

// Target-independent nodes, but with target-specific formats.
def callseq_start : SDNode<"ISD::CALLSEQ_START", SDT_CallSeqStart,
                           [SDNPHasChain, SDNPOutGlue]>;
def callseq_end   : SDNode<"ISD::CALLSEQ_END", SDT_CallSeqEnd,
                           [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

// Target-dependent nodes.
def ltumnfriscv_call : SDNode<"LtumnfRISCVISD::CALL", SDT_LtumnfRISCVCall,
                              [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue,
                               SDNPVariadic]>;

def ltumnfriscv_ret_flag : SDNode<"LtumnfRISCVISD::RET_FLAG", SDTNone,
                                [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

//===----------------------------------------------------------------------===//
// Operand and SDNode transformation definitions.
//===----------------------------------------------------------------------===//

// FIXME: here we simply defines `simm21_lsb0_jal`, as an instance of
//        the `Operand` class, which represent branch target operands.
//        We should check if it is a 21-bit signed immediate
//        where the least significant bit is zero.
def simm21_lsb0_jal : Operand<OtherVT>; // `OtherVT` defined in ValueTypes.td

// ditto
def simm13_lsb0 : Operand<OtherVT>;

def uimm5 : Operand<XLenVT>, ImmLeaf<XLenVT, [{return isUInt<5>(Imm);}]>;

def simm12 : Operand<XLenVT>, ImmLeaf<XLenVT, [{return isInt<12>(Imm);}]>;

def uimm20 : Operand<XLenVT>, ImmLeaf<XLenVT, [{return isUInt<20>(Imm);}]>;

def simm32 : Operand<XLenVT>, ImmLeaf<XLenVT, [{return isInt<32>(Imm);}]>;

// Extract least significant 12 bits from an immediate value and sign extend
// them.
def LO12Sext : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(SignExtend64<12>(N->getZExtValue()),
                                   SDLoc(N), N->getValueType(0));
}]>;

// Extract the most significant 20 bits from an immediate value. Add 1 if bit
// 11 is 1, to compensate for the low 12 bits in the matching immediate addi
// or ld/st being negative.
def HI20 : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(((N->getZExtValue()+0x800) >> 12) & 0xfffff,
                                   SDLoc(N), N->getValueType(0));
}]>;

// FIXME: ditto need check
// A bare symbol used in call only.
def call_symbol : Operand<XLenVT>;

// Addressing modes.
// Necessary because a frameindex can't be matched directly in a pattern.
def FrameAddrRegImm : ComplexPattern<iPTR, 2, "SelectFrameAddrRegImm",
                                     [frameindex, or, add]>;
def AddrRegImm : ComplexPattern<iPTR, 2, "SelectAddrRegImm">;

//===----------------------------------------------------------------------===//
// Instructions
//===----------------------------------------------------------------------===//

// The following opcode names and match those given in Table 19.1 in the
// RISC-V User-level ISA specification ("RISC-V base opcode map").
class RISCVOpcode<bits<7> val> {
  bits<7> Value = val;
}
def OPC_LOAD      : RISCVOpcode<0b0000011>;
def OPC_LOAD_FP   : RISCVOpcode<0b0000111>;
def OPC_MISC_MEM  : RISCVOpcode<0b0001111>;
def OPC_OP_IMM    : RISCVOpcode<0b0010011>;
def OPC_AUIPC     : RISCVOpcode<0b0010111>;
def OPC_OP_IMM_32 : RISCVOpcode<0b0011011>;
def OPC_STORE     : RISCVOpcode<0b0100011>;
def OPC_STORE_FP  : RISCVOpcode<0b0100111>;
def OPC_AMO       : RISCVOpcode<0b0101111>;
def OPC_OP        : RISCVOpcode<0b0110011>;
def OPC_LUI       : RISCVOpcode<0b0110111>;
def OPC_OP_32     : RISCVOpcode<0b0111011>;
def OPC_MADD      : RISCVOpcode<0b1000011>;
def OPC_MSUB      : RISCVOpcode<0b1000111>;
def OPC_NMSUB     : RISCVOpcode<0b1001011>;
def OPC_NMADD     : RISCVOpcode<0b1001111>;
def OPC_OP_FP     : RISCVOpcode<0b1010011>;
def OPC_BRANCH    : RISCVOpcode<0b1100011>;
def OPC_JALR      : RISCVOpcode<0b1100111>;
def OPC_JAL       : RISCVOpcode<0b1101111>;
def OPC_SYSTEM    : RISCVOpcode<0b1110011>;

class RVInst<dag outs, dag ins, string opcodestr, string argstr,
             list<dag> pattern>
    : Instruction {
  field bits<32> Inst;
  let Size = 4;
  let Namespace = "LtumnfRISCV";

  bits<7> Opcode = 0;
  let Inst{6-0} = Opcode;

  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString = opcodestr # "\t" # argstr;
  let Pattern = pattern;
}

// Pseudo instructions
class Pseudo<dag outs, dag ins, list<dag> pattern, string opcodestr = "", string argstr = "">
    : RVInst<outs, ins, opcodestr, argstr, pattern> {
  let isPseudo = 1;
  let isCodeGenOnly = 1;
}

class RVInstR<bits<7> funct7, bits<3> funct3, RISCVOpcode opcode, dag outs,
              dag ins, string opcodestr, string argstr, list<dag> pattern>
    : RVInst<outs, ins, opcodestr, argstr, pattern> {
  bits<5> rs2;
  bits<5> rs1;
  bits<5> rd;

  let Inst{31-25} = funct7;
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-7} = rd;
  let Opcode = opcode.Value;
}

class RVInstI<bits<3> funct3, RISCVOpcode opcode, dag outs, dag ins,
              string opcodestr, string argstr, list<dag> pattern>
    : RVInst<outs, ins, opcodestr, argstr, pattern> {
  bits<12> imm12;
  bits<5> rs1;
  bits<5> rd;

  let Inst{31-20} = imm12;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-7} = rd;
  let Opcode = opcode.Value;
}

class RVInstIShift<bits<5> imm11_7, bits<3> funct3, RISCVOpcode opcode,
                   dag outs, dag ins, string opcodestr, string argstr,
                   list<dag> pattern>
    : RVInst<outs, ins, opcodestr, argstr, pattern> {
  bits<6> shamt;
  bits<5> rs1;
  bits<5> rd;

  let Inst{31-27} = imm11_7;
  let Inst{26} = 0;
  let Inst{25-20} = shamt;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-7} = rd;
  let Opcode = opcode.Value;
}

class RVInstS<bits<3> funct3, RISCVOpcode opcode, dag outs, dag ins,
              string opcodestr, string argstr, list<dag> pattern>
    : RVInst<outs, ins, opcodestr, argstr, pattern> {
  bits<12> imm12;
  bits<5> rs2;
  bits<5> rs1;

  let Inst{31-25} = imm12{11-5};
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-7} = imm12{4-0};
  let Opcode = opcode.Value;
}

class RVInstB<bits<3> funct3, RISCVOpcode opcode, dag outs, dag ins,
              string opcodestr, string argstr, list<dag> pattern>
    : RVInst<outs, ins, opcodestr, argstr, pattern> {
  bits<12> imm12;
  bits<5> rs2;
  bits<5> rs1;

  let Inst{31} = imm12{11};
  let Inst{30-25} = imm12{9-4};
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-8} = imm12{3-0};
  let Inst{7} = imm12{10};
  let Opcode = opcode.Value;
}

class RVInstU<RISCVOpcode opcode, dag outs, dag ins, string opcodestr,
              string argstr, list<dag> pattern>
    : RVInst<outs, ins, opcodestr, argstr, pattern> {
  bits<20> imm20;
  bits<5> rd;

  let Inst{31-12} = imm20;
  let Inst{11-7} = rd;
  let Opcode = opcode.Value;
}

class RVInstJ<RISCVOpcode opcode, dag outs, dag ins, string opcodestr,
              string argstr, list<dag> pattern>
    : RVInst<outs, ins, opcodestr, argstr, pattern> {
  bits<20> imm20;
  bits<5> rd;

  let Inst{31} = imm20{19};
  let Inst{30-21} = imm20{9-0};
  let Inst{20} = imm20{10};
  let Inst{19-12} = imm20{18-11};
  let Inst{11-7} = rd;
  let Opcode = opcode.Value;
}

// Note: `i32imm` defined in Target.td, `timm` defined in TargetSelectionDAG.td
// ADJCALLSTACKDOWN/UP implicitly use/def SP because they may be expanded into
// a stack adjustment and the codegen must know that they may modify the stack
// pointer before prolog-epilog rewriting occurs.
// Pessimistically assume the stack pointer will be clobbered.
// FIXME?: i64imm sync with SDT_CallSeqStart SDTCisVT<0, i64>, SDTCisVT<1, i64>
let Defs = [X2], Uses = [X2] in {
def ADJCALLSTACKDOWN : Pseudo<(outs), (ins i64imm:$amt1, i64imm:$amt2),
                              [(callseq_start timm:$amt1, timm:$amt2)],
                              "#ADJCALLSTACKDOWN", "$amt1 $amt2">;
def ADJCALLSTACKUP   : Pseudo<(outs), (ins i64imm:$amt1, i64imm:$amt2),
                              [(callseq_end timm:$amt1, timm:$amt2)],
                              "#ADJCALLSTACKUP", "$amt1 $amt2">;
}

/// Shifts Instructions

def SLL  : RVInstR<0b0000000, 0b001, OPC_OP,
                  (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                  "sll", "$rd, $rs1, $rs2",
                  []>;
def : Pat<(shl GPR:$rs1, GPR:$rs2), (SLL GPR:$rs1, GPR:$rs2)>;

def SLLI : RVInstIShift<0b00000, 0b001, OPC_OP_IMM,
                        (outs GPR:$rd), (ins GPR:$rs1, uimm5:$shamt),
                        "slli", "$rd, $rs1, $shamt",
                        []>;
def : Pat<(shl GPR:$rs1, simm12:$imm12), (SLLI GPR:$rs1, simm12:$imm12)>;

def SRL : RVInstR<0b0000000, 0b101, OPC_OP,
                  (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                  "srl", "$rd, $rs1, $rs2",
                  []>;
def : Pat<(srl GPR:$rs1, GPR:$rs2), (SRL GPR:$rs1, GPR:$rs2)>;

def SRLI : RVInstIShift<0b00000, 0b101, OPC_OP_IMM,
                        (outs GPR:$rd), (ins GPR:$rs1, uimm5:$shamt),
                        "srli", "$rd, $rs1, $shamt",
                        []>;
def : Pat<(srl GPR:$rs1, simm12:$imm12), (SRLI GPR:$rs1, simm12:$imm12)>;

def SRA : RVInstR<0b0100000, 0b101, OPC_OP,
                  (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                  "sra", "$rd, $rs1, $rs2",
                  []>;
def : Pat<(sra GPR:$rs1, GPR:$rs2), (SRA GPR:$rs1, GPR:$rs2)>;

def SRAI : RVInstIShift<0b01000, 0b101, OPC_OP_IMM,
                        (outs GPR:$rd), (ins GPR:$rs1, uimm5:$shamt),
                        "srai", "$rd, $rs1, $shamt",
                        []>;
def : Pat<(sra GPR:$rs1, simm12:$imm12), (SRAI GPR:$rs1, simm12:$imm12)>;


/// Arithmetic Instructions

def ADD : RVInstR<0b0000000, 0b000, OPC_OP, (outs GPR:$rd),
                  (ins GPR:$rs1, GPR:$rs2), "add", "$rd, $rs1, $rs2",
                  [(set GPR:$rd, (add GPR:$rs1, GPR:$rs2))]>;

def ADDI : RVInstI<0b000, OPC_OP_IMM, (outs GPR:$rd),
                   (ins GPR:$rs1, simm12:$imm12), "addi", "$rd, $rs1, $imm12",
                   [(set GPR:$rd, (add GPR:$rs1, simm12:$imm12))]>;

def SUB : RVInstR<0b0100000, 0b000, OPC_OP, (outs GPR:$rd),
                  (ins GPR:$rs1, GPR:$rs2), "sub", "$rd, $rs1, $rs2",
                  [(set GPR:$rd, (sub GPR:$rs1, GPR:$rs2))]>;

def LUI : RVInstU<OPC_LUI, (outs GPR:$rd), (ins uimm20:$imm20),
                  "lui", "$rd, $imm20", []>;

def AUIPC : RVInstU<OPC_AUIPC, (outs GPR:$rd), (ins uimm20:$imm20),
                    "auipc", "$rd, $imm20", []>;


/// Logical Instructions

def XOR : RVInstR<0b0000000, 0b100, OPC_OP,
                  (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                  "xor", "$rd, $rs1, $rs2",
                  []>;
def : Pat<(xor GPR:$rs1, GPR:$rs2), (XOR GPR:$rs1, GPR:$rs2)>;

def OR : RVInstR<0b0000000, 0b110, OPC_OP,
         (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
         "or", "$rd, $rs1, $rs2",
         []>;
def : Pat<(or GPR:$rs1, GPR:$rs2), (OR GPR:$rs1, GPR:$rs2)>;

def AND : RVInstR<0b0000000, 0b111, OPC_OP,
                  (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                  "and", "$rd, $rs1, $rs2",
                  []>;
def : Pat<(and GPR:$rs1, GPR:$rs2), (AND GPR:$rs1, GPR:$rs2)>;

def XORI : RVInstI<0b100, OPC_OP_IMM,
                   (outs GPR:$rd), (ins GPR:$rs1, simm12:$imm12),
                   "xori", "$rd, $rs1, $imm12",
                   []>;
def : Pat<(xor GPR:$rs1, simm12:$imm12), (XORI GPR:$rs1, simm12:$imm12)>;

def ORI : RVInstI<0b110, OPC_OP_IMM,
                  (outs GPR:$rd), (ins GPR:$rs1, simm12:$imm12),
                  "ori", "$rd, $rs1, $imm12",
                  []>;
def : Pat<(or GPR:$rs1, simm12:$imm12), (ORI GPR:$rs1, simm12:$imm12)>;

def ANDI : RVInstI<0b111, OPC_OP_IMM,
                   (outs GPR:$rd), (ins GPR:$rs1, simm12:$imm12),
                   "andi", "$rd, $rs1, $imm12",
                   []>;
def : Pat<(and GPR:$rs1, simm12:$imm12), (ANDI GPR:$rs1, simm12:$imm12)>;


/// Compare Instructions

def SLT : RVInstR<0b0000000, 0b010, OPC_OP,
                  (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                  "slt", "$rd, $rs1, $rs2",
                  []>;
def : Pat<(setlt GPR:$rs1, GPR:$rs2), (SLT GPR:$rs1, GPR:$rs2)>;

def SLTI : RVInstI<0b010, OPC_OP_IMM,
                   (outs GPR:$rd), (ins GPR:$rs1, simm12:$imm12),
                   "slti", "$rd, $rs1, $imm12",
                   []>;
def : Pat<(setlt GPR:$rs1, simm12:$imm12), (SLTI GPR:$rs1, simm12:$imm12)>;

def SLTU : RVInstR<0b0000000, 0b011, OPC_OP,
          (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
          "sltu", "$rd, $rs1, $rs2",
          []>;
def : Pat<(setult GPR:$rs1, GPR:$rs2), (SLTU GPR:$rs1, GPR:$rs2)>;

def SLTIU : RVInstI<0b011, OPC_OP_IMM,
                   (outs GPR:$rd), (ins GPR:$rs1, simm12:$imm12),
                   "sltiu", "$rd, $rs1, $imm12",
                   []>;
def : Pat<(setult GPR:$rs1, simm12:$imm12), (SLTIU GPR:$rs1, simm12:$imm12)>;


/// Branches Instructions

// Match `brcc` and lower to the appropriate RISC-V branch instruction.
multiclass BrccPat<CondCode Cond, RVInstB Inst> {
  def : Pat<(brcc Cond, GPR:$rs1, GPR:$rs2, bb:$imm12),
            (Inst GPR:$rs1, GPR:$rs2, simm13_lsb0:$imm12)>;
  // Explicitly select 0 to X0. The register coalescer doesn't always do it.
  def : Pat<(brcc Cond, GPR:$rs1, 0, bb:$imm12),
            (Inst GPR:$rs1, X0, simm13_lsb0:$imm12)>;
}

let isBranch = 1, isTerminator = 1 in
def BEQ : RVInstB<0b000, OPC_BRANCH, (outs),
                  (ins GPR:$rs1, GPR:$rs2, simm13_lsb0:$imm12),
                  "beq", "$rs1, $rs2, $imm12",
                  []>;
defm : BrccPat<SETEQ, BEQ>;

let isBranch = 1, isTerminator = 1 in
def BNE : RVInstB<0b001, OPC_BRANCH, (outs),
                  (ins GPR:$rs1, GPR:$rs2, simm13_lsb0:$imm12),
                  "bne", "$rs1, $rs2, $imm12",
                  []>;
defm : BrccPat<SETNE, BNE>;

let isBranch = 1, isTerminator = 1 in
def BLT : RVInstB<0b100, OPC_BRANCH, (outs),
                  (ins GPR:$rs1, GPR:$rs2, simm13_lsb0:$imm12),
                  "blt", "$rs1, $rs2, $imm12",
                  []>;
defm : BrccPat<SETLT, BLT>;

let isBranch = 1, isTerminator = 1 in
def BGE : RVInstB<0b101, OPC_BRANCH, (outs),
                  (ins GPR:$rs1, GPR:$rs2, simm13_lsb0:$imm12),
                  "bge", "$rs1, $rs2, $imm12",
                  []>;
defm : BrccPat<SETGE, BGE>;

let isBranch = 1, isTerminator = 1 in
def BLTU : RVInstB<0b110, OPC_BRANCH, (outs),
                  (ins GPR:$rs1, GPR:$rs2, simm13_lsb0:$imm12),
                  "bltu", "$rs1, $rs2, $imm12",
                  []>;
defm : BrccPat<SETULT, BLTU>;

let isBranch = 1, isTerminator = 1 in
def BGEU : RVInstB<0b111, OPC_BRANCH, (outs),
                  (ins GPR:$rs1, GPR:$rs2, simm13_lsb0:$imm12),
                  "bgeu", "$rs1, $rs2, $imm12",
                  []>;
defm : BrccPat<SETUGE, BGEU>;

def : InstAlias<"beqz $rs, $offset",
                (BEQ GPR:$rs,      X0, simm13_lsb0:$offset)>;
def : InstAlias<"bnez $rs, $offset",
                (BNE GPR:$rs,      X0, simm13_lsb0:$offset)>;
def : InstAlias<"blez $rs, $offset",
                (BGE      X0, GPR:$rs, simm13_lsb0:$offset)>;
def : InstAlias<"bgez $rs, $offset",
                (BGE GPR:$rs,      X0, simm13_lsb0:$offset)>;
def : InstAlias<"bltz $rs, $offset",
                (BLT GPR:$rs,      X0, simm13_lsb0:$offset)>;
def : InstAlias<"bgtz $rs, $offset",
                (BLT      X0, GPR:$rs, simm13_lsb0:$offset)>;


/// Jump & Link Instructions

let isCall = 1 in
def JAL : RVInstJ<OPC_JAL, (outs GPR:$rd), (ins simm21_lsb0_jal:$imm20),
                  "jal", "$rd, $imm20", []>;

let isCall = 1 in
def JALR : RVInstI<0b000, OPC_JALR, (outs GPR:$rd),
                   (ins GPR:$rs1, simm12:$imm12),
                   "jalr", "$rd, ${imm12}(${rs1})",
                   []>;


/// Loads and Stores instructions

def LB : RVInstI<0b000, OPC_LOAD, (outs GPR:$rd), (ins GPR:$rs1, simm12:$imm12),
                 "lb", "$rd, ${imm12}(${rs1})", []>;
def : Pat<(sextloadi8 (AddrRegImm GPR:$rs1, simm12:$imm12)),
          (LB GPR:$rs1, simm12:$imm12)>;
def : Pat<(extloadi8 (AddrRegImm GPR:$rs1, simm12:$imm12)),
          (LB GPR:$rs1, simm12:$imm12)>;

def LH : RVInstI<0b001, OPC_LOAD, (outs GPR:$rd), (ins GPR:$rs1, simm12:$imm12),
                 "lh", "$rd, ${imm12}(${rs1})", []>;
def : Pat<(sextloadi16 (AddrRegImm GPR:$rs1, simm12:$imm12)),
          (LH GPR:$rs1, simm12:$imm12)>;
def : Pat<(extloadi16 (AddrRegImm GPR:$rs1, simm12:$imm12)),
          (LH GPR:$rs1, simm12:$imm12)>;

def LBU : RVInstI<0b100, OPC_LOAD, (outs GPR:$rd), (ins GPR:$rs1, simm12:$imm12),
                 "lbu", "$rd, ${imm12}(${rs1})", []>;
def : Pat<(zextloadi8 (AddrRegImm GPR:$rs1, simm12:$imm12)),
          (LBU GPR:$rs1, simm12:$imm12)>;

def LHU : RVInstI<0b101, OPC_LOAD, (outs GPR:$rd), (ins GPR:$rs1, simm12:$imm12),
                 "lhu", "$rd, ${imm12}(${rs1})", []>;
def : Pat<(zextloadi16 (AddrRegImm GPR:$rs1, simm12:$imm12)),
          (LHU GPR:$rs1, simm12:$imm12)>;

def LW : RVInstI<0b010, OPC_LOAD, (outs GPR:$rd), (ins GPR:$rs1, simm12:$imm12),
                 "lw", "$rd, ${imm12}(${rs1})", []>;
def : Pat<(i64 (sextloadi32 (AddrRegImm GPR:$rs1, simm12:$imm12))),
          (LW GPR:$rs1, simm12:$imm12)>;
def : Pat<(i64 (extloadi32 (AddrRegImm GPR:$rs1, simm12:$imm12))),
          (LW GPR:$rs1, simm12:$imm12)>;

def LWU : RVInstI<0b110, OPC_LOAD, (outs GPR:$rd), (ins GPR:$rs1, simm12:$imm12),
                 "lwu", "$rd, ${imm12}(${rs1})", []>;
def : Pat<(i64 (zextloadi32 (AddrRegImm GPR:$rs1, simm12:$imm12))),
          (LWU GPR:$rs1, simm12:$imm12)>;

def LD : RVInstI<0b011, OPC_LOAD, (outs GPR:$rd),
                 (ins GPR:$rs1, simm12:$imm12), "ld", "$rd, ${imm12}(${rs1})",
                 []>;
def : Pat<(i64 (load (AddrRegImm GPR:$rs1, simm12:$imm12))),
          (LD GPR:$rs1, simm12:$imm12)>;

def SB : RVInstS<0b000, OPC_STORE, (outs),
                 (ins GPR:$rs2, GPR:$rs1, simm12:$imm12),
                 "sb", "$rs2, ${imm12}(${rs1})", []>;
def : Pat<(truncstorei8 GPR:$rs2, (AddrRegImm GPR:$rs1, simm12:$imm12)),
          (SB GPR:$rs2, GPR:$rs1, simm12:$imm12)>;

def SH : RVInstS<0b001, OPC_STORE, (outs),
                 (ins GPR:$rs2, GPR:$rs1, simm12:$imm12),
                 "sh", "$rs2, ${imm12}(${rs1})", []>;
def : Pat<(truncstorei16 GPR:$rs2, (AddrRegImm GPR:$rs1, simm12:$imm12)),
          (SH GPR:$rs2, GPR:$rs1, simm12:$imm12)>;

def SW : RVInstS<0b010, OPC_STORE, (outs),
                 (ins GPR:$rs2, GPR:$rs1, simm12:$imm12),
                 "sh", "$rs2, ${imm12}(${rs1})", []>;
def : Pat<(truncstorei32 (i64 GPR:$rs2), (AddrRegImm GPR:$rs1, simm12:$imm12)),
          (SW GPR:$rs2, GPR:$rs1, simm12:$imm12)>;

def SD : RVInstS<0b011, OPC_STORE, (outs),
                 (ins GPR:$rs2, GPR:$rs1, simm12:$imm12),
                 "sd", "$rs2, ${imm12}(${rs1})",
                 []>;
def : Pat<(store (i64 GPR:$rs2), (AddrRegImm GPR:$rs1, simm12:$imm12)),
          (SD GPR:$rs2, GPR:$rs1, simm12:$imm12)>;


/// Multiply Instructions

def MUL : RVInstR<0b0000001, 0b000, OPC_OP,
                  (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                  "mul", "$rd, $rs1, $rs2",
                  []>;

def MULH : RVInstR<0b0000001, 0b001, OPC_OP,
                   (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                   "mulh", "$rd, $rs1, $rs2",
                   []>;

def MULHSU : RVInstR<0b0000001, 0b010, OPC_OP,
                     (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                     "mulhsu", "$rd, $rs1, $rs2",
                     []>;

def MULHU : RVInstR<0b0000001, 0b011, OPC_OP,
                    (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                    "mulhu", "$rd, $rs1, $rs2",
                    []>;

def : Pat<(mul GPR:$rs1, GPR:$rs2), (MUL GPR:$rs1, GPR:$rs2)>;
def : Pat<(mulhs GPR:$rs1, GPR:$rs2), (MULH GPR:$rs1, GPR:$rs2)>;
// FIXME: def : Pat<(ltumnfriscv_mulhsu GPR:$rs1, GPR:$rs2), (MULHSU GPR:$rs1, GPR:$rs2)>;
def : Pat<(mulhu GPR:$rs1, GPR:$rs2), (MULHU GPR:$rs1, GPR:$rs2)>;


/// Divide Instructions

def DIV : RVInstR<0b0000001, 0b100, OPC_OP, (outs GPR:$rd),
                  (ins GPR:$rs1, GPR:$rs2), "div", "$rd, $rs1, $rs2",
                  []>;

def DIVU : RVInstR<0b0000001, 0b101, OPC_OP, (outs GPR:$rd),
                  (ins GPR:$rs1, GPR:$rs2), "divu", "$rd, $rs1, $rs2",
                  []>;

def REM : RVInstR<0b0000001, 0b110, OPC_OP, (outs GPR:$rd),
                  (ins GPR:$rs1, GPR:$rs2), "rem", "$rd, $rs1, $rs2",
                  []>;

def REMU : RVInstR<0b0000001, 0b111, OPC_OP, (outs GPR:$rd),
                  (ins GPR:$rs1, GPR:$rs2), "remu", "$rd, $rs1, $rs2",
                  []>;

def : Pat<(sdiv GPR:$rs1, GPR:$rs2), (DIV GPR:$rs1, GPR:$rs2)>;
def : Pat<(udiv GPR:$rs1, GPR:$rs2), (DIVU GPR:$rs1, GPR:$rs2)>;
def : Pat<(srem GPR:$rs1, GPR:$rs2), (REM GPR:$rs1, GPR:$rs2)>;
def : Pat<(urem GPR:$rs1, GPR:$rs2), (REMU GPR:$rs1, GPR:$rs2)>;


/// Pseudo Instructions

def : InstAlias<"li $rd, $imm",  (ADDI GPR:$rd, X0, simm12:$imm)>;
def : InstAlias<"mv $rd, $rs",   (ADDI GPR:$rd, GPR:$rs, 0)>;
def : InstAlias<"j $offset", (JAL X0, simm21_lsb0_jal:$offset)>;
def : InstAlias<"ret", (JALR X0, X1, 0), 4>;

// Note:
// - `bb` : SelectionDAG node defined in TargetSelectionDAG.td
// - isBarrier : the specified instruction stops control flow from executing
//               the instruction immediately following it.
let isBarrier = 1, isBranch = 1, isTerminator = 1 in
def PseudoBR : Pseudo<(outs), (ins simm21_lsb0_jal:$imm20), [(br bb:$imm20)]>,
               PseudoInstExpansion<(JAL X0, simm21_lsb0_jal:$imm20)>;

let isBarrier = 1, isReturn = 1, isTerminator = 1 in
def PseudoRET : Pseudo<(outs), (ins), [(ltumnfriscv_ret_flag)]>,
                PseudoInstExpansion<(JALR X0, X1, 0)>;

// PseudoCALL is a pseudo instruction which will eventually expand to auipc
// and jalr while encoding.
let isCall = 1, Defs = [X1], isCodeGenOnly = 0, Size = 8 in
def PseudoCALL : Pseudo<(outs), (ins call_symbol:$func), []> {
  let AsmString = "call\t$func";
}

// Note: tglobaladdr defined in TargetSelectionDAG.td
def : Pat<(ltumnfriscv_call tglobaladdr:$func), (PseudoCALL tglobaladdr:$func)>;


/// Codegen patterns

def : Pat<(simm12:$imm), (ADDI X0, simm12:$imm)>;
// TODO: Add a pattern for immediates with all zeroes in the lower 12 bits.
def : Pat<(simm32:$imm), (ADDI (LUI (HI20 imm:$imm)), (LO12Sext imm:$imm))>;
