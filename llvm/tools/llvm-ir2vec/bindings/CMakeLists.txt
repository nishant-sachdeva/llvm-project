find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(pybind11 CONFIG QUIET)

if(NOT pybind11_FOUND)
  message(WARNING "pybind11 not found - skipping py_ir2vec")
  return()
endif()

pybind11_add_module(py_ir2vec
  ir2vec_bindings.cpp
)
