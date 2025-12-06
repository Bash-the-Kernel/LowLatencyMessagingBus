#!/bin/bash
set -e

echo "Building LowLatencyBus..."

mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

echo ""
echo "Build complete! Executables in build/"
echo ""
echo "Run tests: ./test_bus"
echo "Run benchmark: ./benchmark"
echo "Run publisher: ./publisher --create"
echo "Run subscriber: ./subscriber"
