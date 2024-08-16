# Compiler and flags
CC=gcc
CXX=g++

# Check if Clang is available
ifneq ($(shell command -v clang 2> /dev/null),)
    CC=clang
    CXX=clang++
endif

# Number of cores to use for compilation
CORES=$(shell expr $(shell nproc --all) / 2)

# Build directory
BUILD_DIR=build

# Default target
all: $(BUILD_DIR)/galaxy_visualization_raylib

# Clean the build directory
clean:
	@echo "Cleaning up"
	@rm -rf $(BUILD_DIR)

# Create the build directory
$(BUILD_DIR):
	@echo "Creating the build directory"
	@mkdir -p $(BUILD_DIR)

# Run CMake and compile
$(BUILD_DIR)/galaxy_visualization_raylib: $(BUILD_DIR) CMakeLists.txt
	@echo "Running CMake"
	@cd $(BUILD_DIR) && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=$(CXX)
	@echo "Compiling galaxy_visualization_raylib"
	@$(MAKE) -C $(BUILD_DIR) -j$(CORES)

# Measure build time, compile, and run
time_run: all
	@start=$$(date +%s); \
	$(MAKE) all; \
	end=$$(date +%s); \
	runtime=$$((end-start)); \
	echo "Build took $$runtime seconds"; \
	$(MAKE) run

# Run the program
run: $(BUILD_DIR)/galaxy_visualization_raylib
	@echo "Running galaxy_visualization_raylib"
	@echo "Executable path: $(BUILD_DIR)/galaxy_visualization_raylib"
	@ls -l $(BUILD_DIR)/galaxy_visualization_raylib
	@$(BUILD_DIR)/galaxy_visualization_raylib || (echo "Failed to run $(BUILD_DIR)/galaxy_visualization_raylib"; exit 1)

.PHONY: all clean run time_run