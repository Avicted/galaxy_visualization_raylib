#!/bin/bash

# Start measuring time
start=`date +%s`

echo "Cleaning up"
echo ""
rm -rf build 2> /dev/null || true

# Use Clang if available, else fall back to GCC
export CC=gcc
export CXX=g++

if command -v clang &> /dev/null
then
    export CC=clang
    export CXX=clang++
fi

echo "Using $CC as the compiler"
echo ""

echo "Creating the build directory"
echo ""
mkdir -p build 2> /dev/null || true

echo "Compiling galaxy_visualization_raylib"
echo ""

# Get number of cores / 2
CORES=$(nproc --all)
CORES=$((CORES / 2))
echo "Using $CORES cores for compilation"

echo "Building Raylib + galaxy_visualization_raylib"

echo ""
cmake -S ./ -B build -DCMAKE_BUILD_TYPE=Release  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_C_COMPILER=$CC -DCMAKE_CXX_COMPILER=$CXX
cmake --build ./build --config Release --target all -- -j $CORES

make -C ./build

# End measuring time
end=`date +%s`

# Calculate time difference
runtime=$((end-start))

# Print time difference
echo ""
echo "Build took $runtime seconds"

# Run the program
echo "Running galaxy_visualization_raylib"
echo ""
./build/galaxy_visualization_raylib
