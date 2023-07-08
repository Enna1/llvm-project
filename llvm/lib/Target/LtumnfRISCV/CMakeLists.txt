add_llvm_component_group(LtumnfRISCV)

set(LLVM_TARGET_DEFINITIONS LtumnfRISCV.td)

tablegen(LLVM LtumnfRISCVGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM LtumnfRISCVGenCallingConv.inc -gen-callingconv)
tablegen(LLVM LtumnfRISCVGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM LtumnfRISCVGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM LtumnfRISCVGenMCPseudoLowering.inc -gen-pseudo-lowering)
tablegen(LLVM LtumnfRISCVGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM LtumnfRISCVGenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(LtumnfRISCVCommonTableGen)

set(sources
  LtumnfRISCVTargetMachine.cpp
  LtumnfRISCVISelDAGToDAG.cpp
  LtumnfRISCVAsmPrinter.cpp
  LtumnfRISCVSubtarget.cpp
  LtumnfRISCVISelLowering.cpp
  LtumnfRISCVFrameLowering.cpp
  LtumnfRISCVInstrInfo.cpp
  LtumnfRISCVRegisterInfo.cpp
  LtumnfRISCVMCInstLower.cpp
)

add_llvm_target(LtumnfRISCVCodeGen
  ${sources}

  LINK_COMPONENTS
  AsmPrinter
  Core
  CodeGen
  MC
  LtumnfRISCVDesc
  LtumnfRISCVInfo
  SelectionDAG
  Support
  Target
  TargetParser

  ADD_TO_COMPONENT
  LtumnfRISCV
)

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
