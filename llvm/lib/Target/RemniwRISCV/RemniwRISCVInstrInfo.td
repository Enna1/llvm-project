class RV64 : Instruction {
  let Size = 4;
  let Namespace = "RemniwRISCV";
  bits<32> Inst;
}

def RemniwRISCVRetFlag : SDNode<"RemniwRISCVISD::RET_FLAG", SDTNone,
                             [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

def simm12 : Operand<i64>, ImmLeaf<i64, [{return isInt<12>(Imm);}]>;

/// Arithmetic Instructions

def ADDI : RV64 {
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

def ADDW : RV64 {
  let OutOperandList = (outs GPR:$rd);
  let InOperandList = (ins GPR:$rs1, GPR:$rs2);
  let AsmString = "addw $rd, $rs1, $rs2";
  let Pattern = [(set GPR:$rd, (add GPR:$rs1, GPR:$rs2))];
  bits<5> rd;
  bits<5> rs1;
  bits<5> rs2;
  let Inst{31-25} = 0b0000000;
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = 0b000;
  let Inst{11-7} = rd;
  let Inst{6-0} = 0b0111011;
}

def SUBW : RV64 {
  let OutOperandList = (outs GPR:$rd);
  let InOperandList = (ins GPR:$rs1, GPR:$rs2);
  let AsmString = "subw $rd, $rs1, $rs2";
  let Pattern = [(set GPR:$rd, (sub GPR:$rs1, GPR:$rs2))];
  bits<5> rd;
  bits<5> rs1;
  bits<5> rs2;
  let Inst{31-25} = 0b0100000;
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = 0b000;
  let Inst{11-7} = rd;
  let Inst{6-0} = 0b0111011;
}

def DIVW : RV64 {
  let OutOperandList = (outs GPR:$rd);
  let InOperandList = (ins GPR:$rs1, GPR:$rs2);
  let AsmString = "mul $rd, $rs1, $rs2";
  let Pattern = [(set GPR:$rd, (sdiv GPR:$rs1, GPR:$rs2))];
  bits<5> rd;
  bits<5> rs1;
  bits<5> rs2;
  let Inst{31-25} = 0b0000001;
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = 0b100;
  let Inst{11-7} = rd;
  let Inst{6-0} = 0b0110011;
}

/// Load and Store instructions

def LD : RV64 {
  let OutOperandList = (outs GPR:$rd);
  let InOperandList = (ins GPR:$rs1, simm12:$imm12);
  let AsmString = "ld $rd, ${imm12}(${rs1})";
  let Pattern = [];
  bits<5> rd;
  bits<5> rs1;
  bits<12> imm12;
  let Inst{31-20} = imm12;
  let Inst{19-15} = rs1;
  let Inst{14-12} = 0b011;
  let Inst{11-7} = rd;
  let Inst{6-0} = 0b0000011;
}

def SD : RV64 {
  let OutOperandList = (outs);
  let InOperandList = (ins GPR:$rs2, GPR:$rs1, simm12:$imm12);
  let AsmString = "sd $rs2, ${imm12}(${rs1})";
  let Pattern = [];
  bits<5> rs2;
  bits<5> rs1;
  bits<12> imm12;
  let Inst{31-25} = imm12{11-5};
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-7} = imm12{4-0};
  let Inst{6-0} = 0b0100011;
}

def : InstAlias<"li $rd, $imm",  (ADDI GPR:$rd, X0, simm12:$imm)>;
def : InstAlias<"mv $rd, $rs",   (ADDI GPR:$rd, GPR:$rs,       0)>;

def JALR : RV64 {
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

def PseudoRET : RV64, PseudoInstExpansion<(JALR X0, X1, 0)> {
    let OutOperandList = (outs);
    let InOperandList = (ins);
    let Pattern = [(RemniwRISCVRetFlag)];
    let isPseudo = 1;
    let isReturn = 1;
    let isTerminator = 1;
}
