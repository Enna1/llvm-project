add_llvm_component_library(LLVMLtumnfRISCVDesc
  LtumnfRISCVMCTargetDesc.cpp
  LtumnfRISCVMCAsmInfo.cpp
  LtumnfRISCVInstPrinter.cpp

  LINK_COMPONENTS
  MC
  LtumnfRISCVInfo
  Support
  TargetParser

  ADD_TO_COMPONENT
  LtumnfRISCV
)
