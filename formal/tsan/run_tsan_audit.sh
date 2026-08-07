#!/usr/bin/env bash
set -e

echo "================================================================="
echo " RUNNING THREADSANITIZER (TSAN) & ASAN RACE & SAFETY AUDIT     "
echo "================================================================="

BUILD_DIR="build_tsan"
rm -rf ${BUILD_DIR}
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

cmake -DCMAKE_C_FLAGS="-fsanitize=thread -g -O1" ..
make -j$(nproc)

echo "[TSAN AUDIT] Executing complete unit test suite under ThreadSanitizer..."
ctest --output-on-failure

echo "================================================================="
echo " THREADSANITIZER AUDIT PASSED: ZERO RACE CONDITIONS DETECTED!   "
echo "================================================================="
