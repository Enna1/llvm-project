add_llvm_component_library(LLVMRemniwRISCVDesc
  RemniwRISCVMCTargetDesc.cpp
  RemniwRISCVMCAsmInfo.cpp
  RemniwRISCVInstPrinter.cpp

  LINK_COMPONENTS
  MC
  RemniwRISCVInfo
  Support

  ADD_TO_COMPONENT
  RemniwRISCV
)
