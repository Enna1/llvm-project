let Namespace = "LtumnfRISCV" in {
class LtumnfRISCVReg<bits<5> enc, string name, list<string> alt = []>
    : Register<name, alt> {
  let HWEncoding{4-0} = enc;
  let AltNames = alt;
}

def ABIRegAltName : RegAltNameIndex;
} // Namespace = "LtumnfRISCV"

let RegAltNameIndices = [ABIRegAltName] in {
  def X0  : LtumnfRISCVReg<0,  "x0",  ["zero"]>, DwarfRegNum<[0]>;
  def X1  : LtumnfRISCVReg<1,  "x1",  ["ra"]>, DwarfRegNum<[1]>;
  def X2  : LtumnfRISCVReg<2,  "x2",  ["sp"]>, DwarfRegNum<[2]>;
  def X3  : LtumnfRISCVReg<3,  "x3",  ["gp"]>, DwarfRegNum<[3]>;
  def X4  : LtumnfRISCVReg<4,  "x4",  ["tp"]>, DwarfRegNum<[4]>;
  def X5  : LtumnfRISCVReg<5,  "x5",  ["t0"]>, DwarfRegNum<[5]>;
  def X6  : LtumnfRISCVReg<6,  "x6",  ["t1"]>, DwarfRegNum<[6]>;
  def X7  : LtumnfRISCVReg<7,  "x7",  ["t2"]>, DwarfRegNum<[7]>;
  def X8  : LtumnfRISCVReg<8,  "x8",  ["s0", "fp"]>, DwarfRegNum<[8]>;
  def X9  : LtumnfRISCVReg<9,  "x9",  ["s1"]>, DwarfRegNum<[9]>;
  def X10 : LtumnfRISCVReg<10, "x10", ["a0"]>, DwarfRegNum<[10]>;
  def X11 : LtumnfRISCVReg<11, "x11", ["a1"]>, DwarfRegNum<[11]>;
  def X12 : LtumnfRISCVReg<12, "x12", ["a2"]>, DwarfRegNum<[12]>;
  def X13 : LtumnfRISCVReg<13, "x13", ["a3"]>, DwarfRegNum<[13]>;
  def X14 : LtumnfRISCVReg<14, "x14", ["a4"]>, DwarfRegNum<[14]>;
  def X15 : LtumnfRISCVReg<15, "x15", ["a5"]>, DwarfRegNum<[15]>;
  def X16 : LtumnfRISCVReg<16, "x16", ["a6"]>, DwarfRegNum<[16]>;
  def X17 : LtumnfRISCVReg<17, "x17", ["a7"]>, DwarfRegNum<[17]>;
  def X18 : LtumnfRISCVReg<18, "x18", ["s2"]>, DwarfRegNum<[18]>;
  def X19 : LtumnfRISCVReg<19, "x19", ["s3"]>, DwarfRegNum<[19]>;
  def X20 : LtumnfRISCVReg<20, "x20", ["s4"]>, DwarfRegNum<[20]>;
  def X21 : LtumnfRISCVReg<21, "x21", ["s5"]>, DwarfRegNum<[21]>;
  def X22 : LtumnfRISCVReg<22, "x22", ["s6"]>, DwarfRegNum<[22]>;
  def X23 : LtumnfRISCVReg<23, "x23", ["s7"]>, DwarfRegNum<[23]>;
  def X24 : LtumnfRISCVReg<24, "x24", ["s8"]>, DwarfRegNum<[24]>;
  def X25 : LtumnfRISCVReg<25, "x25", ["s9"]>, DwarfRegNum<[25]>;
  def X26 : LtumnfRISCVReg<26, "x26", ["s10"]>, DwarfRegNum<[26]>;
  def X27 : LtumnfRISCVReg<27, "x27", ["s11"]>, DwarfRegNum<[27]>;
  def X28 : LtumnfRISCVReg<28, "x28", ["t3"]>, DwarfRegNum<[28]>;
  def X29 : LtumnfRISCVReg<29, "x29", ["t4"]>, DwarfRegNum<[29]>;
  def X30 : LtumnfRISCVReg<30, "x30", ["t5"]>, DwarfRegNum<[30]>;
  def X31 : LtumnfRISCVReg<31, "x31", ["t6"]>, DwarfRegNum<[31]>;
}

defvar RV64 = DefaultMode;
def XLenVT : ValueTypeByHwMode<[RV64], [i64]>;
def XLenRI : RegInfoByHwMode<[RV64], [RegInfo<64,64,64>]>;

// The order of registers represents the preferred allocation sequence.
// Registers are listed in the order caller-save, callee-save, specials.
def GPR : RegisterClass<"LtumnfRISCV", [XLenVT], /*alignment*/ 64, (add
  (sequence "X%u", 10, 17),
  (sequence "X%u", 5, 7),
  (sequence "X%u", 28, 31),
  (sequence "X%u", 8, 9),
  (sequence "X%u", 18, 27),
  (sequence "X%u", 0, 4)
)> {
  let RegInfos = XLenRI;
}
