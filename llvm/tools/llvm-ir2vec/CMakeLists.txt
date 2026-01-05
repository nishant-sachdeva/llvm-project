set(LLVM_LINK_COMPONENTS
  Analysis
  Core
  Demangle
  IRReader
  Support
  CodeGen
  MIRParser
  AllTargetsAsmParsers
  AllTargetsCodeGens
  AllTargetsDescs
  AllTargetsInfos
  TargetParser
)

# Build the main executable
add_llvm_tool(llvm-ir2vec
  llvm-ir2vec.cpp
  utils.cpp
  DEPENDS intrinsics_gen
)

#===----------------------------------------------------------------------===#
# Python Bindings
#===----------------------------------------------------------------------===#

find_package(Python3 COMPONENTS Interpreter Development.Module)
find_package(pybind11 CONFIG QUIET)

if(NOT Python3_FOUND)
  message(STATUS "Python3 not found - skipping Python bindings")
  return()
endif()

if(NOT pybind11_FOUND)
  message(STATUS "pybind11 not found - skipping Python bindings")
  return()
endif()

message(STATUS "Building Python bindings")

#===----------------------------------------------------------------------===#
# py_ir2vec - Python bindings for IR2Vec
#===----------------------------------------------------------------------===#

pybind11_add_module(py_ir2vec MODULE 
  bindings/ir2vec_bindings.cpp 
  utils.cpp
)

target_link_libraries(py_ir2vec PRIVATE 
  LLVMAnalysis
  LLVMCore
  LLVMSupport
  LLVMIRReader
  LLVMAsmParser
  LLVMPasses
  LLVMDemangle
)

target_include_directories(py_ir2vec PRIVATE 
  ${LLVM_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(py_ir2vec PRIVATE ${LLVM_DEFINITIONS})

# target_compile_options(py_ir2vec PRIVATE -fexceptions -frtti)