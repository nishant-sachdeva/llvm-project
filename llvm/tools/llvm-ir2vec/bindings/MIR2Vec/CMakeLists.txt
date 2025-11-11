find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(pybind11 CONFIG QUIET)

if(NOT pybind11_FOUND)
  message(WARNING "pybind11 not found - skipping py_mir2vec")
  return()
endif()

pybind11_add_module(py_mir2vec MODULE 
  mir2vec_bindings.cpp 
  ../../src/MIR2VecTool.cpp
)

target_include_directories(py_mir2vec PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# Manually expand AllTargets* since pybind11_add_module doesn't handle them
set(ALL_TARGET_LIBS "")
foreach(target ${LLVM_TARGETS_TO_BUILD})
  list(APPEND ALL_TARGET_LIBS
    LLVM${target}AsmParser
    LLVM${target}CodeGen
    LLVM${target}Desc
    LLVM${target}Info
  )
endforeach()

target_link_libraries(py_mir2vec PRIVATE 
  LLVMAnalysis
  LLVMCore
  LLVMDemangle
  LLVMIRReader
  LLVMSupport
  LLVMCodeGen
  LLVMMIRParser
  LLVMTargetParser
  ${ALL_TARGET_LIBS}
)