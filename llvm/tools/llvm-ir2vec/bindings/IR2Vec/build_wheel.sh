#!/usr/bin/env bash
set -euo pipefail

echo "======================================"
echo "Building py-ir2vec Python Wheel"
echo "======================================"

# Determine paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LLVM_SOURCE="$(cd "${SCRIPT_DIR}/../../../../.." && pwd)"
BUILD_LLVM="${LLVM_SOURCE}/../build-llvm"
BINDINGS_SOURCE="${SCRIPT_DIR}"
WHEEL_OUTPUT_DIR="${SCRIPT_DIR}/wheels"

echo "LLVM Source: ${LLVM_SOURCE}"
echo "Build LLVM Dir: ${BUILD_LLVM}"
echo "Bindings Source: ${BINDINGS_SOURCE}"
echo "Wheel Output: ${WHEEL_OUTPUT_DIR}"

# Verify required files exist
if [ ! -f "${BINDINGS_SOURCE}/pyproject.toml" ]; then
    echo "ERROR: pyproject.toml not found"
    exit 1
fi

if [ ! -f "${BINDINGS_SOURCE}/__init__.py" ]; then
    echo "ERROR: __init__.py not found"
    exit 1
fi

# Find .so file
SO_FILE=$(find "${BUILD_LLVM}/lib" -name "py_ir2vec*.so" | head -n1)
if [ -z "$SO_FILE" ]; then
    echo "ERROR: py_ir2vec*.so not found in ${BUILD_LLVM}/lib/"
    echo "Did you run build_llvm.sh first?"
    exit 1
fi

echo "✓ Found pyproject.toml"
echo "✓ Found __init__.py"
echo "✓ Found .so: $(basename ${SO_FILE})"

# Create temp build directory
TEMP_BUILD=$(mktemp -d)
trap "rm -rf ${TEMP_BUILD}" EXIT
echo "✓ Created temp dir: ${TEMP_BUILD}"

# Copy all files from bindings/
echo ""
echo "Preparing wheel build structure..."
cp -r "${BINDINGS_SOURCE}"/* "${TEMP_BUILD}/"
echo "✓ Copied bindings files"

# Create package directory and copy .so and __init__.py
mkdir -p "${TEMP_BUILD}/py_ir2vec"
cp "${SO_FILE}" "${TEMP_BUILD}/py_ir2vec/"
cp "${BINDINGS_SOURCE}/__init__.py" "${TEMP_BUILD}/py_ir2vec/"
echo "✓ Copied .so and __init__.py to package directory"

# Build wheel
echo ""
echo "Building wheel..."
rm -rf "${WHEEL_OUTPUT_DIR}"
mkdir -p "${WHEEL_OUTPUT_DIR}"

cd "${TEMP_BUILD}"
python3 -m pip wheel . --no-deps -w "${WHEEL_OUTPUT_DIR}"

# Repair wheel with auditwheel
UNREPAIRED=$(ls "${WHEEL_OUTPUT_DIR}"/*.whl 2>/dev/null | tail -n1)
auditwheel repair "${UNREPAIRED}" -w "${WHEEL_OUTPUT_DIR}" 2>/dev/null && rm "${UNREPAIRED}" || true

# Verify wheel was created
WHEEL_FILE=$(ls "${WHEEL_OUTPUT_DIR}"/*.whl 2>/dev/null | tail -n1)
if [ -z "${WHEEL_FILE}" ]; then
    echo "ERROR: Wheel build failed - no .whl file found"
    exit 1
fi

echo ""
echo "✓ Wheel built successfully!"
echo "  File: $(basename ${WHEEL_FILE})"
echo "  Size: $(du -h ${WHEEL_FILE} | cut -f1)"
echo ""
echo "Install with: pip install ${WHEEL_FILE}"