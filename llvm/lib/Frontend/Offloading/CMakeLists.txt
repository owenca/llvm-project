add_llvm_component_library(LLVMFrontendOffloading
  Utility.cpp
  OffloadWrapper.cpp
  PropertySet.cpp

  ADDITIONAL_HEADER_DIRS
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Frontend

  DEPENDS
  intrinsics_gen

  LINK_COMPONENTS
  Core
  BinaryFormat
  Object
  ObjectYAML
  Support
  TransformUtils
  TargetParser
)
