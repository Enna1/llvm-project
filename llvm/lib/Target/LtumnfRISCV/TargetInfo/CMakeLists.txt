add_llvm_component_library(LLVMLtumnfRISCVInfo
  LtumnfRISCVTargetInfo.cpp

  LINK_COMPONENTS
  MC
  Support
  TargetParser

  ADD_TO_COMPONENT
  LtumnfRISCV
)
