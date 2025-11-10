//===- ir2vec_bindings.cpp - Python Bindings for IR2Vec ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "IR2VecTool.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <fstream>
#include <memory>
#include <string>

namespace py = pybind11;
using namespace llvm;
using namespace llvm::ir2vec;

namespace llvm {
namespace ir2vec {
void setIR2VecVocabPath(StringRef Path);
StringRef getIR2VecVocabPath();
} // namespace ir2vec
} // namespace llvm

namespace {

bool fileNotValid(const std::string &Filename) {
  std::ifstream F(Filename, std::ios_base::in | std::ios_base::binary);
  return !F.good();
}

std::unique_ptr<Module> getLLVMIR(const std::string &Filename,
                                   LLVMContext &Context) {
  SMDiagnostic Err;
  auto M = parseIRFile(Filename, Err, Context);
  if (!M) {
    Err.print(Filename.c_str(), outs());
    throw std::runtime_error("Failed to parse IR file.");
  }
  return M;
}

class PyIR2VecTool {
private:
  std::unique_ptr<LLVMContext> Ctx;
  std::unique_ptr<Module> M;
  std::unique_ptr<IR2VecTool> Tool;

public:
  PyIR2VecTool(std::string Filename, std::string Mode,
								std::string VocabOverride) {
		if (fileNotValid(Filename))
			throw std::runtime_error("Invalid file path");

		if (Mode != "sym" && Mode != "fa")
			throw std::runtime_error("Invalid mode. Use 'sym' or 'fa'");

		Ctx = std::make_unique<LLVMContext>();
		M = getLLVMIR(Filename, *Ctx);
		Tool = std::make_unique<IR2VecTool>(*M);

		if (VocabOverride.empty())
			throw std::runtime_error("Error - Empty Vocab Path not allowed");

		setIR2VecVocabPath(VocabOverride);

		bool Ok = Tool->initializeVocabulary();
		if (!Ok)
			throw std::runtime_error("Failed to initialize IR2Vec vocabulary");
  }

  py::dict getFuncVectorMap() {
    auto result = Tool->getFunctionEmbeddings();
    py::dict py_result;

    for (const auto &[demangled_name, name_emb_pair] : result) {
      const auto &[actual_name, embedding] = name_emb_pair;
      auto data = embedding.getData();
      
      py::array_t<double> np_array(data.size(), data.data());
      py_result[py::str(demangled_name)] = py::make_tuple(py::str(actual_name), np_array);
    }

    return py_result;
  }

  py::list generateBBEmbeddings() {
    auto embeddings = Tool->getBBEmbeddings();
    py::list result;
    
    for (const auto &[bb_name, emb] : embeddings) {
      auto data = emb.getData();
      py::array_t<double> np_array(data.size(), data.data());
      result.append(py::make_tuple(py::str(bb_name), np_array));
    }
    
    return result;
  }

  py::list generateInstEmbeddings() {
    auto embeddings = Tool->getInstEmbeddings();
    py::list result;
    
    for (const auto &[inst_str, emb] : embeddings) {
      auto data = emb.getData();
      py::array_t<double> np_array(data.size(), data.data());
      result.append(py::make_tuple(py::str(inst_str), np_array));
    }

    return result;
  }

  py::dict generateTriplets() {
    auto result = Tool->getTriplets();
    py::list triplets_list;
    for (const auto& t : result.Triplets) {
      triplets_list.append(py::make_tuple(t.Head, t.Tail, t.Relation));
    }

    return py::dict(
      py::arg("max_relation") = result.MaxRelation,
      py::arg("triplets") = triplets_list
    );
  }
};

} // namespace

PYBIND11_MODULE(py_ir2vec, m) {
  m.doc() = R"pbdoc(
  Python bindings for LLVM IR2Vec

  IR2Vec provides distributed representations for LLVM IR.
  Example:
    >>> import py_ir2vec as M
    >>> tool = M.initEmbedding(
      filename=tesy.ll,
      mode="sym",
      vocab_override=vocab_path
    )
  )pbdoc";

  // Main tool class
  py::class_<PyIR2VecTool>(m, "IR2VecTool")
		.def(py::init<std::string, std::string, std::string>(),
			py::arg("filename"), py::arg("mode"), py::arg("vocab_override"),
			"Initialize IR2Vec on an LLVM IR file\n"
			"Args:\n"
			"  filename: Path to LLVM IR file (.bc or .ll)\n"
			"  mode: 'sym' for symbolic (default) or 'fa' for flow-aware\n"
			"  vocab_override: Path to vocabulary file")
		.def("getFuncVectorMap", &PyIR2VecTool::getFuncVectorMap,
			"Generate function-level embeddings for all functions\n"
			"Returns: dict[str, tuple[str, numpy.ndarray[float64]]] - "
			"{demangled_name: (actual_name, embedding)}")
		.def("generateBBEmbeddings", &PyIR2VecTool::generateBBEmbeddings,
			"Generate basic block embeddings for all functions\n"
			"Returns: list[tuple[str, numpy.ndarray[float64]]]")
		.def("generateInstEmbeddings", &PyIR2VecTool::generateInstEmbeddings,
      "Generate instruction embeddings for all functions\n"
      "Returns: list[tuple[str, numpy.ndarray[float64]]]")
		.def("generateTriplets", &PyIR2VecTool::generateTriplets,
			"Generate triplets for vocabulary training\n"
			"Returns: TripletResult Dict with max_relation and list of triplets");

  m.def(
		"initEmbedding",
		[](std::string filename, std::string mode, std::string vocab_override) {
			return std::make_unique<PyIR2VecTool>(filename, mode, vocab_override);
		},
		py::arg("filename"), py::arg("mode") = "sym",
		py::arg("vocab_override") = "",
		"Initialize IR2Vec on an LLVM IR file and return an IR2VecTool\n"
		"Args:\n"
		"  filename: Path to LLVM IR file (.bc or .ll)\n"
		"  mode: 'sym' for symbolic (default) or 'fa' for flow-aware\n"
		"  vocab_override: Optional path to vocabulary file\n"
		"Returns: IR2VecTool instance");

  m.def(
    "getEntityMappings",
    &IR2VecTool::getEntityMappings,
    "Get entity mappings (vocabulary)\n"
    "Returns: list[str] - list of entity names where index is entity_id");
}