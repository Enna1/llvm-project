class RemniwRISCVReg<bits<5> enc, string name, list<string> alt = []>
    : Register<name, alt> {
  let HWEncoding{4-0} = enc;
  let Namespace = "RemniwRISCV";
}

def ABIRegAltName : RegAltNameIndex;

let RegAltNameIndices = [ABIRegAltName] in {
  def X0  : RemniwRISCVReg<0,  "x0",  ["zero"]>;
  def X1  : RemniwRISCVReg<1,  "x1",  ["ra"]>;
  def X2  : RemniwRISCVReg<2,  "x2",  ["sp"]>;
  def X3  : RemniwRISCVReg<3,  "x3",  ["gp"]>;
  def X4  : RemniwRISCVReg<4,  "x4",  ["tp"]>;
  def X5  : RemniwRISCVReg<5,  "x5",  ["t0"]>;
  def X6  : RemniwRISCVReg<6,  "x6",  ["t1"]>;
  def X7  : RemniwRISCVReg<7,  "x7",  ["t2"]>;
  def X8  : RemniwRISCVReg<8,  "x8",  ["s0", "fp"]>;
  def X9  : RemniwRISCVReg<9,  "x9",  ["s1"]>;
  def X10 : RemniwRISCVReg<10, "x10", ["a0"]>;
  def X11 : RemniwRISCVReg<11, "x11", ["a1"]>;
  def X12 : RemniwRISCVReg<12, "x12", ["a2"]>;
  def X13 : RemniwRISCVReg<13, "x13", ["a3"]>;
  def X14 : RemniwRISCVReg<14, "x14", ["a4"]>;
  def X15 : RemniwRISCVReg<15, "x15", ["a5"]>;
  def X16 : RemniwRISCVReg<16, "x16", ["a6"]>;
  def X17 : RemniwRISCVReg<17, "x17", ["a7"]>;
  def X18 : RemniwRISCVReg<18, "x18", ["s2"]>;
  def X19 : RemniwRISCVReg<19, "x19", ["s3"]>;
  def X20 : RemniwRISCVReg<20, "x20", ["s4"]>;
  def X21 : RemniwRISCVReg<21, "x21", ["s5"]>;
  def X22 : RemniwRISCVReg<22, "x22", ["s6"]>;
  def X23 : RemniwRISCVReg<23, "x23", ["s7"]>;
  def X24 : RemniwRISCVReg<24, "x24", ["s8"]>;
  def X25 : RemniwRISCVReg<25, "x25", ["s9"]>;
  def X26 : RemniwRISCVReg<26, "x26", ["s10"]>;
  def X27 : RemniwRISCVReg<27, "x27", ["s11"]>;
  def X28 : RemniwRISCVReg<28, "x28", ["t3"]>;
  def X29 : RemniwRISCVReg<29, "x29", ["t4"]>;
  def X30 : RemniwRISCVReg<30, "x30", ["t5"]>;
  def X31 : RemniwRISCVReg<31, "x31", ["t6"]>;
}

// The order of registers represents the preferred allocation sequence.
// Registers are listed in the order caller-save, callee-save, specials.
def GPR : RegisterClass<"RemniwRISCV", /*XLenVT for RV64*/ [i64], /*alignment*/ 64, (add
  (sequence "X%u", 10, 17),
  (sequence "X%u", 5, 7),
  (sequence "X%u", 28, 31),
  (sequence "X%u", 8, 9),
  (sequence "X%u", 18, 27),
  (sequence "X%u", 0, 4)
)>;
