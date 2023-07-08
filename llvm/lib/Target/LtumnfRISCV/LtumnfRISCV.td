include "llvm/Target/Target.td"

include "LtumnfRISCVRegisterInfo.td"
include "LtumnfRISCVCallingConv.td"
include "LtumnfRISCVInstrInfo.td"

def : ProcessorModel<"generic-rv64", NoSchedModel, []>;

def LtumnfRISCVInstrInfo : InstrInfo;

def LtumnfRISCV : Target {
    let InstructionSet = LtumnfRISCVInstrInfo;
}
