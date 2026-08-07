#!/usr/bin/env bash
set -e

echo "================================================================="
echo " RUNNING ADDRESSSANITIZER (ASAN) MEMORY SAFETY AUDIT             "
echo "================================================================="

BUILD_DIR="build_asan"
rm -rf ${BUILD_DIR}
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

cmake -DCMAKE_C_FLAGS="-fsanitize=address -g -O1" ..
make -j$(nproc)

echo "[ASAN AUDIT] Executing complete unit test suite under AddressSanitizer..."
ctest --output-on-failure

echo "================================================================="
echo " ADDRESSSANITIZER AUDIT PASSED: ZERO MEMORY LEAKS OR ERRORS!    "
echo "================================================================="
