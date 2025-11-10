find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(pybind11 CONFIG QUIET)

if(NOT pybind11_FOUND)
  message(WARNING "pybind11 not found - skipping py_ir2vec")
  return()
endif()

pybind11_add_module(py_ir2vec MODULE 
  ir2vec_bindings.cpp 
  ../../src/IR2VecTool.cpp
)

target_include_directories(py_ir2vec PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
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