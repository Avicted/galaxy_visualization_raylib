#!/bin/bash

set -e

# Parse arguments
EMBED_ASSETS=false
CLEAN=false
RUN=false

for arg in "$@"; do
    case $arg in
        --embed|--embedded)
            EMBED_ASSETS=true
            ;;
        --clean)
            CLEAN=true
            ;;
        --run)
            RUN=true
            ;;
        --help|-h)
            echo "Usage: ./build.sh [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --embed, --embedded  Build single-binary with all assets embedded"
            echo "  --clean              Clean build directory before building"
            echo "  --run                Run the executable after building"
            echo "  --help, -h           Show this help message"
            echo ""
            echo "Examples:"
            echo "  ./build.sh                    # Normal build with external assets"
            echo "  ./build.sh --embed            # Build single portable binary"
            echo "  ./build.sh --embed --run      # Build embedded and run"
            exit 0
            ;;
    esac
done

CC=clang
CFLAGS="-std=c99 -O3 -march=native -Wall -Wextra -Werror -Wpedantic"
LDLIBS="-lm -lraylib"

SRC_FILES=("./src/unity_build.c")
INCLUDE_DIRS="-I./include -I./vendor"
OUTPUT_DIR="./build"
EXECUTABLE_NAME="galaxy_visualization_raylib"

if [ "$EMBED_ASSETS" = true ]; then
    OUTPUT_DIR="./build_embedded"
    CFLAGS="$CFLAGS -DEMBED_ASSETS"
    INCLUDE_DIRS="$INCLUDE_DIRS -I$OUTPUT_DIR"
fi

# Clean if requested or if switching build types
if [ "$CLEAN" = true ] || [ -d "$OUTPUT_DIR" ]; then
    rm -rf "$OUTPUT_DIR"
fi

mkdir -p "$OUTPUT_DIR"

if [ "$EMBED_ASSETS" = true ]; then
    echo "=== Generating embedded asset headers... ==="
    ./scripts/generate_embedded.sh "$OUTPUT_DIR/embedded"
    echo ""
    echo "=== Building with embedded assets... ==="
else
    echo "=== Building with external assets... ==="
    cp -r assets "$OUTPUT_DIR/"
fi

$CC $CFLAGS $INCLUDE_DIRS -o "$OUTPUT_DIR/$EXECUTABLE_NAME" "${SRC_FILES[@]}" $LDLIBS

echo ""
echo "=== Build complete! ==="
ls -lh "$OUTPUT_DIR/$EXECUTABLE_NAME"

if [ "$EMBED_ASSETS" = true ]; then
    echo ""
    echo "Single portable binary created! Can run from anywhere:"
    echo "  $OUTPUT_DIR/$EXECUTABLE_NAME"
else
    echo ""
    echo "Run from project directory:"
    echo "  $OUTPUT_DIR/$EXECUTABLE_NAME"
fi

if [ "$RUN" = true ]; then
    echo ""
    echo "=== Running... ==="
    "$OUTPUT_DIR/$EXECUTABLE_NAME"
fi
