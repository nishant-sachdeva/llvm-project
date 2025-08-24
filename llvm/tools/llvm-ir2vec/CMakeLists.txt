set(LLVM_LINK_COMPONENTS
  Analysis
  Core
  Demangle
  IRReader
  Support
  )

add_llvm_tool(llvm-ir2vec
  llvm-ir2vec.cpp
  IR2VecTool.cpp
  )

add_subdirectory(bindings)