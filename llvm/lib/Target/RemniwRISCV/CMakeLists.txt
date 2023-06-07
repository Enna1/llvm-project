add_llvm_component_group(RemniwRISCV)

add_llvm_target(RemniwRISCVCodeGen
  RemniwRISCVTargetMachine.cpp

  LINK_COMPONENTS
  RemniwRISCVDesc
  RemniwRISCVInfo

  ADD_TO_COMPONENT
  RemniwRISCV
)

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
