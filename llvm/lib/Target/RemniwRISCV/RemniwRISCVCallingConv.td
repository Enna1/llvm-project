// https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-cc.adoc

def RetCC_RemniwRISCV : CallingConv<[
  // Promote i1/i8/i16/i32 arguments to i64.
  CCIfType<[i1, i8, i16, i32], CCPromoteToType<i64>>,

  // GPR x10, x17 return values registers
  CCIfType<[i64], CCAssignToReg<[X10, X11]>>

  // FIXME: CCIfType<[i64], CCAssignToStack<8, 8>>
]>;

def CC_RemniwRISCV : CallingConv<[
  // Promote i1/i8/i16/i32 arguments to i64.
  CCIfType<[i1, i8, i16, i32], CCPromoteToType<i64>>,

  // GPR x10 - x17 Argument registers
  CCIfType<[i64], CCAssignToReg<[X10, X11, X12, X13, X14, X15, X16, X17]>>,

  // If no register was found -> assign to stack
  CCIfType<[i64], CCAssignToStack<8, 8>>
]>;

def CSR : CalleeSavedRegs<(add X2, X8, X9, (sequence "X%u", 18, 27))>;
