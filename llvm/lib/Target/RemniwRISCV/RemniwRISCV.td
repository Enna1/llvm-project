include "llvm/Target/Target.td"

include "RemniwRISCVRegisterInfo.td"
include "RemniwRISCVCallingConv.td"
include "RemniwRISCVInstrInfo.td"

def : ProcessorModel<"generic-rv64", NoSchedModel, []>;

def RemniwRISCVInstrInfo : InstrInfo;

def RemniwRISCV : Target {
    let InstructionSet = RemniwRISCVInstrInfo;
}
