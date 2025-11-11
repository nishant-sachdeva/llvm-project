#!/usr/bin/env bash
set -euo pipefail

echo "======================================"
echo "Building LLVM with py_ir2vec"
echo "======================================"

# Determine paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LLVM_SOURCE="$(cd "${SCRIPT_DIR}/../../../../.." && pwd)"
BUILD_DIR="${LLVM_SOURCE}/../build-llvm"

echo "LLVM Source: ${LLVM_SOURCE}"
echo "Build Dir: ${BUILD_DIR}"

mkdir -p "${BUILD_DIR}"

# Configure LLVM
echo ""
echo "Configuring LLVM..."
cmake -G Ninja \
    -S "${LLVM_SOURCE}/llvm" \
    -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_TARGETS_TO_BUILD=host \
    -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_INCLUDE_BENCHMARKS=OFF \
    -DLLVM_INCLUDE_EXAMPLES=OFF \
    -DPython3_EXECUTABLE=python3 \
-Dpybind11_DIR=$(python3 -m pybind11 --cmakedir)

# Build py_ir2vec
echo ""
echo "Building py_ir2vec..."
ninja -C "${BUILD_DIR}" py_ir2vec

# Verify build
SO_FILE=$(find "${BUILD_DIR}/lib" -name "py_ir2vec*.so" | head -n1)
if [ -z "$SO_FILE" ]; then
    echo "ERROR: py_ir2vec.so not found!"
    exit 1
fi

echo ""
echo "✓ Build successful: $(basename ${SO_FILE})"