include "llvm/Target/Target.td"

include "RemniwRISCVRegisterInfo.td"
include "RemniwRISCVInstrInfo.td"
include "RemniwRISCVCallingConv.td"

def : ProcessorModel<"generic-rv64", NoSchedModel, []>;

def RemniwRISCVInstrInfo : InstrInfo;

def RemniwRISCV : Target {
    let InstructionSet = RemniwRISCVInstrInfo;
}
