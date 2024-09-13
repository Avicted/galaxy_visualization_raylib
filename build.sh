#!/bin/bash

set -xe

# Remove the build directory if it exists
rm -rf build/ > /dev/null 2>&1

# Create a build directory
mkdir -p build > /dev/null 2>&1

# Copy all the files from src and includes to the build directory
cp -r src/* build/
cp -r includes/* build/  # Add this line to copy the includes directory

# Build with g++
g++ build/frontend.cpp -o galaxy_visualization_raylib -lraylib

# Run the executable
./galaxy_visualization_raylib
