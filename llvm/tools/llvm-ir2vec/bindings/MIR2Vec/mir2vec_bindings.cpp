//===- mir2vec_bindings.cpp - Python Bindings for MIR2Vec ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MIR2VecTool.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <fstream>
#include <memory>
#include <string>

namespace py = pybind11;
using namespace llvm;
using namespace llvm::mir2vec;

namespace llvm {
namespace mir2vec {
void setMIR2VecVocabPath(std::string Path);
std::string getMIR2VecVocabPath();
} // namespace mir2vec
} // namespace llvm

namespace {

bool fileNotValid(const std::string &Filename) {
  std::ifstream F(Filename, std::ios_base::in | std::ios_base::binary);
  return !F.good();
}

class PyMIR2VecTool {
private:
  mir2vec::MIRContext Ctx;
  std::unique_ptr<mir2vec::MIR2VecTool> Tool;
  std::string Mode;
  bool VocabInitialized;
  
  void verifyMIR2VecVocabulary() {
    assert(Tool->getVocabulary() &&
      "MIR2Vec vocabulary should be initialized at this point");

    LLVM_DEBUG(dbgs() << "MIR2Vec vocabulary loaded successfully.\n"
      << "Vocabulary dimension: "
      << Tool->getVocabulary()->getDimension() << "\n"
      << "Vocabulary size: "
      << Tool->getVocabulary()->getCanonicalSize() << "\n"
    ); 
  }

  void initializeMIR2VecVocabularyForEmb(std::string vocab_path) {
    setMIR2VecVocabPath(vocab_path);
      if (!Tool->initializeVocabulary(*Ctx.M)) {
        throw std::runtime_error("Failed to initialize MIR2Vec vocabulary");
      }

      verifyMIR2VecVocabulary();
  }

  MachineFunction* getMachineFunction(std::string FunctionName) {
    Function *F = Ctx.M->getFunction(FunctionName);
    if (!F || F->isDeclaration())
      throw std::runtime_error("Function not found or is a declaration: " + FunctionName);

    MachineFunction *MF = Ctx.MMI->getMachineFunction(*F);
    if (!MF)
      throw std::runtime_error("No MachineFunction found for: " + FunctionName);

    return MF;
  }

  template<typename KeyType>
  py::list generateEmbeddings(
    std::string FunctionName,
    std::string vocab_path,
    std::function<std::vector<std::pair<KeyType, Embedding>>
      (mir2vec::MIR2VecTool&, MachineFunction&)> getEmbeddingsFn
  ) {
  
    throw std::runtime_error("Function Not Implemented");
    initializeMIR2VecVocabularyForEmb(vocab_path);
    MachineFunction *MF = getMachineFunction(FunctionName);

    auto embeddings = getEmbeddingsFn(*Tool, *MF);
    py::list result;
    
    for (const auto &[key, emb] : embeddings) {
      auto data = emb.getData();
      py::array_t<double> np_array(data.size(), data.data());
      result.append(py::make_tuple(py::str(key), np_array));
    }

    return result;
  }

public:
  PyMIR2VecTool(std::string Filename, std::string Mode)
      : Mode(Mode), VocabInitialized(false) {
    if (fileNotValid(Filename))
      throw std::runtime_error("Invalid file path");

    if (Mode != "sym") // && Mode != "fa")
      throw std::runtime_error("Invalid mode. Only 'sym' mode supported.");

    static codegen::RegisterCodeGenFlags CGF;
    // Setup MIR context

    auto Err = mir2vec::setupMIRContext(Filename, Ctx);
    if (Err) {
      std::string ErrMsg;
      raw_string_ostream OS(ErrMsg);
      OS << "Failed to setup MIR context: " << toString(std::move(Err));
      throw std::runtime_error(OS.str());
    }

    // Create MIR2Vec tool
    Tool = std::make_unique<mir2vec::MIR2VecTool>(*Ctx.MMI);
  }

  py::list getEntityMappings() {
    if (!Tool->initializeVocabularyForLayout(*Ctx.M))
      throw std::runtime_error("Failed to initialize MIR2Vec vocabulary");
    verifyMIR2VecVocabulary();

    auto entities = Tool->getEntityMappings();
    py::list py_entities;
    
    for (const auto &entity : entities) {
      py_entities.append(py::str(entity));
    }
    
    return py_entities;
  }

  py::dict generateTriplets() {
    if (!Tool->initializeVocabularyForLayout(*Ctx.M))
      throw std::runtime_error("Failed to initialize MIR2Vec vocabulary");
    verifyMIR2VecVocabulary();

    auto result = Tool->getTriplets(*Ctx.M);

    py::list triplets_list;
    for (const auto& t : result.Triplets) {
      triplets_list.append(py::make_tuple(t.Head, t.Tail, t.Relation));
    }
    
    return py::dict(
      py::arg("max_relation") = result.MaxRelation,
      py::arg("triplets") = triplets_list
    );
  }

  py::dict getFuncVectorMap(std::string vocab_path = "") {
    throw std::runtime_error("Function Not Implemented");
    initializeMIR2VecVocabularyForEmb(vocab_path);
    auto result = Tool->getFunctionEmbeddings(*Ctx.M);
    py::dict py_result;

    for (const auto &[demangled_name, name_emb_pair] : result) {
      const auto &[actual_name, embedding] = name_emb_pair;
      auto data = embedding.getData();
      
      py::array_t<double> np_array(data.size(), data.data());
      py_result[py::str(demangled_name)] = py::make_tuple(py::str(actual_name), np_array);
    }

    return py_result;
  }

  py::list generateMBBEmbeddings(std::string FunctionName, std::string vocab_path = "") {
    return generateEmbeddings<std::string>(FunctionName, vocab_path,
      std::function<BBVecList(mir2vec::MIR2VecTool&, MachineFunction&)>(
        [](mir2vec::MIR2VecTool& tool, MachineFunction& mf) { return tool.getMBBEmbeddings(mf); }));
  }

  py::list generateMInstEmbeddings(std::string FunctionName, std::string vocab_path = "") {
    return generateEmbeddings<std::string>(FunctionName, vocab_path,
      std::function<InstVecList(mir2vec::MIR2VecTool&, MachineFunction&)>(
        [](mir2vec::MIR2VecTool& tool, MachineFunction& mf) { return tool.getMInstEmbeddings(mf); }));
  }
};

} // namespace

PYBIND11_MODULE(py_mir2vec, m) {
  m.doc() = R"pbdoc(
  Python bindings for LLVM MIR2Vec

  MIR2Vec provides distributed representations for LLVM Machine IR.
  Example:
    >>> import py_mir2vec as M
    >>> tool = M.initEmbedding(
          filename="test.mir",
          mode="sym",
          vocab_path="/path/to/vocab"
        )
    >>> entities = tool.getEntityMappings()
    >>> embeddings = tool.getFuncVectorMap()
  )pbdoc";

  // Main tool class
  py::class_<PyMIR2VecTool>(m, "MIR2VecTool")
    .def(py::init<std::string, std::string>(),
      py::arg("filename"), 
      py::arg("mode") = "sym",
      "Initialize MIR2Vec on a Machine IR file\n"
      "Args:\n"
      "  filename: Path to MIR file (.mir)\n"
      "  mode: 'sym' for symbolic (default) or 'fa' for flow-aware (not yet supported)")
    .def("getEntityMappings", &PyMIR2VecTool::getEntityMappings,
      "Get entity mappings (vocabulary)\n"
      "Returns: list[str] - list of entity names where index is entity_id")
    .def("generateTriplets", &PyMIR2VecTool::generateTriplets,
      "Generate triplets for vocabulary training for all functions\n"
      "Returns: TripletResult Dict with max_relation and list of triplets")
    .def("getFuncVectorMap", &PyMIR2VecTool::getFuncVectorMap,
      py::arg("vocab_path") = "",
      "Generate function-level embeddings for all functions\n"
      "Returns: dict[str, tuple[str, numpy.ndarray[float64]]] - "
      "{demangled_name: (actual_name, embedding)}")
    .def("generateMBBEmbeddings", &PyMIR2VecTool::generateMBBEmbeddings,
      py::arg("function_name"),
      py::arg("vocab_path") = "",
      "Generate machine basic block embeddings for a specific function\n"
      "Args:\n"
      "  function_name: Name of the function\n"
      "Returns: list[tuple[str, numpy.ndarray[float64]]]")
    .def("generateMInstEmbeddings", &PyMIR2VecTool::generateMInstEmbeddings,
      py::arg("function_name"),
      py::arg("vocab_path") = "",
      "Generate machine instruction embeddings for a specific function\n"
      "Args:\n"
      "  function_name: Name of the function\n"
      "Returns: list[tuple[str, numpy.ndarray[float64]]]");

  m.def(
    "initEmbedding",
    [](std::string filename, std::string mode) {
      return std::make_unique<PyMIR2VecTool>(filename, mode);
    },
    py::arg("filename"), 
    py::arg("mode") = "sym");
}