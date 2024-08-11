# Compiler and flags
CC=gcc
CXX=g++

# Check if Clang is available
ifneq ($(shell command -v clang 2> /dev/null),)
    CC=clang
    CXX=clang++
endif

# Number of cores to use
CORES=$(shell expr $(shell nproc --all) / 2)

# Build directory
BUILD_DIR=build

# Default target
all: clean $(BUILD_DIR)/galaxy_visualization_raylib run

# Clean the build directory
clean:
	@echo "Cleaning up"
	@rm -rf $(BUILD_DIR)

# Create the build directory
$(BUILD_DIR):
	@echo "Creating the build directory"
	@mkdir -p $(BUILD_DIR)

# Run CMake and compile
$(BUILD_DIR)/galaxy_visualization_raylib: $(BUILD_DIR)
	@echo "Compiling galaxy_visualization_raylib"
	@cmake -S ./ -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_C_COMPILER=$(CC) -DCMAKE_CXX_COMPILER=$(CXX)
	@cmake --build $(BUILD_DIR) --config Release --target all -- -j $(CORES)

# Run the program
run:
	@echo "Running galaxy_visualization_raylib"
	@./$(BUILD_DIR)/galaxy_visualization_raylib

# Measure build time and run
time: clean
	@start=$$(date +%s); \
	make all; \
	end=$$(date +%s); \
	runtime=$$$$((end-start)); \
	echo "Build and run took $$$$runtime seconds"

.PHONY: all clean run time
