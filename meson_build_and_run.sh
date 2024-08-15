#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Define versions and directories
RAYLIB_REPO="https://github.com/raysan5/raylib.git"
RAYLIB_TAG="5.0"
RAYLIB_DIR="raylib"
BUILD_DIR="build"

# Function to print messages
print_message() {
    echo ">>> $1"
}

print_message "Checking for Meson and Ninja..."
if ! command -v meson &> /dev/null || ! command -v ninja &> /dev/null; then
    print_message "Meson and Ninja are required but not found. Please install them before proceeding."
    exit 1
fi

# Clone Raylib if it does not already exist
if [ ! -d "$RAYLIB_DIR" ]; then
    print_message "Cloning Raylib repository..."
    git clone --branch $RAYLIB_TAG --depth 1 $RAYLIB_REPO $RAYLIB_DIR
else
    print_message "Raylib directory already exists. Skipping clone."
fi

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    print_message "Creating build directory..."
    mkdir -p $BUILD_DIR
fi

# Run Meson setup
print_message "Setting up the Meson build system..."
meson setup $BUILD_DIR --buildtype=release

# Build the project
print_message "Building the project..."
cd $BUILD_DIR
ninja -j$(nproc)

print_message "Build process completed successfully."

cd ..

# Run the project
# Check if the executable file exists
if [ -f "./build/galaxy_visualization_raylib" ]; then
    print_message "Running the project..."
    ./build/galaxy_visualization_raylib
else
    print_message "Executable file not found. Please ensure that the build process completed successfully."
fi
