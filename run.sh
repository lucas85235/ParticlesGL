#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

echo "==================================="
echo "    Building ParticleGL...         "
echo "==================================="

# Configure the project
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON

# Build the project using all available cores
cmake --build build -j$(nproc)

echo ""
echo "==================================="
echo "    Running Tests...               "
echo "==================================="

cd build
ctest --output-on-failure
cd ..

echo ""
echo "==================================="
echo "    Running ParticleGL...          "
echo "==================================="

./build/ParticleGL
