cmake_minimum_required(VERSION 3.20)
project(py_ir2vec LANGUAGES C CXX)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(pybind11 CONFIG REQUIRED)
find_package(LLVM REQUIRED CONFIG)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

llvm_map_components_to_libnames(IR2VEC_LLVM_LIBS
  Analysis Core Support IRReader AsmParser Passes Demangle)

pybind11_add_module(py_ir2vec MODULE ir2vec_bindings.cpp ../../src/IR2VecTool.cpp)
target_include_directories(py_ir2vec PRIVATE ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
target_link_libraries(py_ir2vec PRIVATE ${IR2VEC_LLVM_LIBS})

install(TARGETS py_ir2vec LIBRARY DESTINATION .)