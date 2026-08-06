# Makefile for BEAM Modular Monolith
BUILD_DIR ?= build

.PHONY: all build test clean train-pgo

all: build

build:
	@cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	@cmake --build $(BUILD_DIR)

test:
	@ctest --test-dir $(BUILD_DIR) --output-on-failure

train-pgo:
	@echo "=== Running Profile-Guided Optimization (PGO) Training Workloads ==="
	@cmake -B $(BUILD_DIR)_pgo_generate -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO_GENERATE=ON
	@cmake --build $(BUILD_DIR)_pgo_generate
	@echo "PGO profile generation build ready."

clean:
	@rm -rf $(BUILD_DIR) $(BUILD_DIR)_pgo_generate
