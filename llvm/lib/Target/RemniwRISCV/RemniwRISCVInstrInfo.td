// Addressing modes.
// Necessary because a frameindex can't be matched directly in a pattern.
def FrameAddrRegImm : ComplexPattern<iPTR, 2, "SelectFrameAddrRegImm",
                                     [frameindex, or, add]>;
def AddrRegImm : ComplexPattern<iPTR, 2, "SelectAddrRegImm">;

// Instructions
class RVInst : Instruction {
  let Size = 4;
  let Namespace = "RemniwRISCV";
  bits<32> Inst;
}

// Target-independent type requirements, but with target-specific formats.
def SDT_CallSeqStart : SDCallSeqStart<[SDTCisVT<0, i32>,
                                       SDTCisVT<1, i32>]>;
def SDT_CallSeqEnd   : SDCallSeqEnd<[SDTCisVT<0, i32>,
                                     SDTCisVT<1, i32>]>;

// Target-dependent type requirements.
def SDT_RemniwRISCVCall     : SDTypeProfile<0, -1, [SDTCisVT<0, /*XLenVT for RV64*/i64>]>;

// Target-independent nodes, but with target-specific formats.
def callseq_start : SDNode<"ISD::CALLSEQ_START", SDT_CallSeqStart,
                           [SDNPHasChain, SDNPOutGlue]>;
def callseq_end   : SDNode<"ISD::CALLSEQ_END", SDT_CallSeqEnd,
                           [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

// Target-dependent nodes.
def remniwriscv_call : SDNode<"RemniwISD::CALL", SDT_RemniwRISCVCall,
                              [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue,
                               SDNPVariadic]>;

def remniwriscv_ret_flag : SDNode<"RemniwRISCVISD::RET_FLAG", SDTNone,
                                [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;


def simm12 : Operand<i64>, ImmLeaf<i64, [{return isInt<12>(Imm);}]>;

// FIXME: here simply defines `simm21_lsb0_jal`,
// instance of the `Operand` class, which represent branch target operands.
// should check if is a 21-bit signed immediate where the least significant bit is zero.
// Note: `OtherVT` is defined in `ValueTypes.td`
def simm21_lsb0_jal : Operand<OtherVT>;
def simm13_lsb0 : Operand<OtherVT>;

// Pessimistically assume the stack pointer will be clobbered
// Note: i32imm defined in Target.td, timm defined in TargetSelectionDAG.td
// FIXME: why need this ?
/*
let Defs = [X2], Uses = [X2], isPseudo = 1, isCodeGenOnly = 1  in {
def ADJCALLSTACKDOWN : RVInst<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                              [(callseq_start timm:$amt1, timm:$amt2)]>;
def ADJCALLSTACKUP   : RVInst<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                              [(callseq_end timm:$amt1, timm:$amt2)]>;
} // Defs = [X2], Uses = [X2]
*/

/// Arithmetic Instructions

def ADDI : RVInst {
  let OutOperandList = (outs GPR:$rd);
  let InOperandList = (ins GPR:$rs1, simm12:$imm12);
  let AsmString = "addi $rd, $rs1, $imm12";
  let Pattern = [(set GPR:$rd, (add GPR:$rs1, simm12:$imm12))];
  bits<5> rd;
  bits<5> rs1;
  bits<12> imm12;
  let Inst{31-20} = imm12;
  let Inst{19-15} = rs1;
  let Inst{14-12} = 0b000;
  let Inst{11-7} = rd;
  let Inst{6-0} = 0b0010011;
}

def ADD : RVInst {
  let OutOperandList = (outs GPR:$rd);
  let InOperandList = (ins GPR:$rs1, GPR:$rs2);
  let AsmString = "add $rd, $rs1, $rs2";
  let Pattern = [(set GPR:$rd, (add GPR:$rs1, GPR:$rs2))];
  bits<5> rd;
  bits<5> rs1;
  bits<5> rs2;
  let Inst{31-25} = 0b0000000; // funct7
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = 0b000; // funct3
  let Inst{11-7} = rd;
  let Inst{6-0} = 0b0110011; // op
}

def SUB : RVInst {
  let OutOperandList = (outs GPR:$rd);
  let InOperandList = (ins GPR:$rs1, GPR:$rs2);
  let AsmString = "sub $rd, $rs1, $rs2";
  let Pattern = [(set GPR:$rd, (sub GPR:$rs1, GPR:$rs2))];
  bits<5> rd;
  bits<5> rs1;
  bits<5> rs2;
  let Inst{31-25} = 0b0100000; // funct7
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = 0b000; // funct3
  let Inst{11-7} = rd;
  let Inst{6-0} = 0b0110011; // op
}

def MUL : RVInst {
  let OutOperandList = (outs GPR:$rd);
  let InOperandList = (ins GPR:$rs1, GPR:$rs2);
  let AsmString = "mul $rd, $rs1, $rs2";
  let Pattern = [(set GPR:$rd, (mul GPR:$rs1, GPR:$rs2))];
  bits<5> rd;
  bits<5> rs1;
  bits<5> rs2;
  let Inst{31-25} = 0b0000001; // funct7
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = 0b000; // funct3
  let Inst{11-7} = rd;
  let Inst{6-0} = 0b0110011; // op
}

def DIV : RVInst {
  let OutOperandList = (outs GPR:$rd);
  let InOperandList = (ins GPR:$rs1, GPR:$rs2);
  let AsmString = "div $rd, $rs1, $rs2";
  let Pattern = [(set GPR:$rd, (sdiv GPR:$rs1, GPR:$rs2))];
  bits<5> rd;
  bits<5> rs1;
  bits<5> rs2;
  let Inst{31-25} = 0b0000001; // funct7
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = 0b100; // funct3
  let Inst{11-7} = rd;
  let Inst{6-0} = 0b0110011; // op
}

/// Load and Store instructions

def LD : RVInst {
  let OutOperandList = (outs GPR:$rd);
  let InOperandList = (ins GPR:$rs1, simm12:$imm12);
  let AsmString = "ld $rd, ${imm12}(${rs1})";
  let Pattern = []; // Pattern for LD is defined below using Pat
  bits<5> rd;
  bits<5> rs1;
  bits<12> imm12;
  let Inst{31-20} = imm12;
  let Inst{19-15} = rs1;
  let Inst{14-12} = 0b011;
  let Inst{11-7} = rd;
  let Inst{6-0} = 0b0000011;
}

def SD : RVInst {
  let OutOperandList = (outs);
  let InOperandList = (ins GPR:$rs2, GPR:$rs1, simm12:$imm12);
  let AsmString = "sd $rs2, ${imm12}(${rs1})";
  let Pattern = []; // Pattern for SD is defined below using Pat
  bits<5> rs2;
  bits<5> rs1;
  bits<12> imm12;
  let Inst{31-25} = imm12{11-5};
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = 0b011;
  let Inst{11-7} = imm12{4-0};
  let Inst{6-0} = 0b0100011;
}

def : Pat<(i64 (load (AddrRegImm GPR:$rs1, simm12:$imm12))),
          (LD GPR:$rs1, simm12:$imm12)>;

def : Pat<(store (i64 GPR:$rs2), (AddrRegImm GPR:$rs1, simm12:$imm12)),
          (SD GPR:$rs2, GPR:$rs1, simm12:$imm12)>;

def : InstAlias<"li $rd, $imm",  (ADDI GPR:$rd, X0, simm12:$imm)>;
def : InstAlias<"mv $rd, $rs",   (ADDI GPR:$rd, GPR:$rs, 0)>;

/// Branch Instructions
// def BEQ  : BranchCC_rri<0b000, "beq">;
// def BNE  : BranchCC_rri<0b001, "bne">;

def BLT : RVInst {
  let OutOperandList = (outs);
  let InOperandList = (ins GPR:$rs1, GPR:$rs2, simm13_lsb0:$imm12);
  let AsmString = "blt $rs1, $rs2, $imm12";
  let Pattern = [];
  let isBranch = 1;
  let isTerminator = 1;
  bits<20> imm12;
  bits<5> rs1;
  bits<5> rs2;
  let Inst{31} = imm12{11};
  let Inst{30-25} = imm12{9-4};
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = 0b100;
  let Inst{11-8} = imm12{3-0};
  let Inst{7} = imm12{10};
  let Inst{6-0} = 0b1100011;
}

def : Pat<(brcc SETLT, GPR:$rs1, GPR:$rs2, bb:$imm12),
            (BLT GPR:$rs1, GPR:$rs2, simm13_lsb0:$imm12)>;

// Explicitly select 0 to X0. The register coalescer doesn't always do it.
def : Pat<(brcc SETLT, GPR:$rs1, 0, bb:$imm12),
            (BLT GPR:$rs1, X0, simm13_lsb0:$imm12)>;

def JAL : RVInst {
  let OutOperandList = (outs GPR:$rd);
  let InOperandList = (ins simm21_lsb0_jal:$imm20);
  let AsmString = "jal $rd, $imm20";
  bits<20> imm20;
  bits<5> rd;
  let Inst{31} = imm20{19};
  let Inst{30-21} = imm20{9-0};
  let Inst{20} = imm20{10};
  let Inst{19-12} = imm20{18-11};
  let Inst{11-7} = rd;
  let Inst{6-0} = 0b1101111;
}

def : InstAlias<"j $offset",   (JAL X0, simm21_lsb0_jal:$offset)>;

def PseudoBR : RVInst, PseudoInstExpansion<(JAL X0, simm21_lsb0_jal:$imm20)> {
    let OutOperandList = (outs);
    let InOperandList = (ins simm21_lsb0_jal:$imm20);
    // Note: bb is a SelectionDAG node defined in TargetSelectionDAG.td
    let Pattern = [(br bb:$imm20)];
    let isPseudo = 1;
    let isBranch = 1;
    let isTerminator = 1;
}

def JALR : RVInst {
  let OutOperandList = (outs GPR:$rd);
  let InOperandList = (ins GPR:$rs1, simm12:$imm12);
  let AsmString = "jalr $rd, ${imm12}(${rs1})";
  bits<5> rd;
  bits<5> rs1;
  bits<12> imm12;
  let Inst{31-20} = imm12;
  let Inst{19-15} = rs1;
  let Inst{14-12} = 0b000;
  let Inst{11-7} = rd;
  let Inst{6-0} = 0b1101111;
  let isCall = 1;
}

def : InstAlias<"ret", (JALR X0, X1, 0), 4>;

def PseudoRET : RVInst, PseudoInstExpansion<(JALR X0, X1, 0)> {
    let OutOperandList = (outs);
    let InOperandList = (ins);
    let Pattern = [(remniwriscv_ret_flag)];
    let isPseudo = 1;
    let isReturn = 1;
    let isTerminator = 1;
}
