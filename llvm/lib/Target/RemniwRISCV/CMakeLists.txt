add_llvm_component_group(RemniwRISCV)

set(LLVM_TARGET_DEFINITIONS RemniwRISCV.td)

tablegen(LLVM RemniwRISCVGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM RemniwRISCVGenCallingConv.inc -gen-callingconv)
tablegen(LLVM RemniwRISCVGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM RemniwRISCVGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM RemniwRISCVGenMCPseudoLowering.inc -gen-pseudo-lowering)
tablegen(LLVM RemniwRISCVGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM RemniwRISCVGenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(RemniwRISCVCommonTableGen)

set(sources
  RemniwRISCVTargetMachine.cpp
  RemniwRISCVISelDAGToDAG.cpp
  RemniwRISCVAsmPrinter.cpp
  RemniwRISCVSubtarget.cpp
  RemniwRISCVISelLowering.cpp
  RemniwRISCVFrameLowering.cpp
  RemniwRISCVInstrInfo.cpp
  RemniwRISCVRegisterInfo.cpp
  RemniwRISCVMCInstLower.cpp
)

add_llvm_target(RemniwRISCVCodeGen
  ${sources}

  LINK_COMPONENTS
  AsmPrinter
  Core
  CodeGen
  MC
  RemniwRISCVDesc
  RemniwRISCVInfo
  SelectionDAG
  Support
  Target
  TargetParser

  ADD_TO_COMPONENT
  RemniwRISCV
)

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
