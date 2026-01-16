#!/bin/bash

set -e

# Parse arguments
EMBED_ASSETS=false
CLEAN=false
RUN=false
STATIC_RAYLIB=false
MINGW=false

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
        --static)
            STATIC_RAYLIB=true
            ;;
        --mingw|--windows)
            MINGW=true
            ;;
        --help|-h)
            echo "Usage: ./build.sh [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --embed, --embedded  Build single-binary with all assets embedded"
            echo "  --static             Link raylib statically (system libs remain dynamic)"
            echo "  --mingw, --windows    Cross-compile Windows .exe (embedded + static raylib)"
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
LDLIBS="-lm"

APP_VERSION=$(sed -nE "s/.*version[[:space:]]*:[[:space:]]*'([^']+)'.*/\1/p" meson.build | head -n 1)
if [ -z "$APP_VERSION" ]; then
    APP_VERSION="0.0.0"
fi
CFLAGS="$CFLAGS -DAPP_VERSION=\"$APP_VERSION\""

PKG_CONFIG_BIN="pkg-config"
RAYLIB_SUBPROJECT_DIR="./subprojects/raylib"
RAYLIB_MAKE_DIR="$RAYLIB_SUBPROJECT_DIR/src"
RAYLIB_STATIC_LIB="$RAYLIB_MAKE_DIR/libraylib.a"

if [ "$STATIC_RAYLIB" = true ]; then
    if [ ! -d "$RAYLIB_SUBPROJECT_DIR" ]; then
        echo "[ERROR] raylib subproject not found. Run: meson subprojects download raylib"
        exit 1
    fi

    if [ ! -f "$RAYLIB_STATIC_LIB" ]; then
        echo "=== Building raylib static library (subproject)... ==="
        make -C "$RAYLIB_MAKE_DIR" PLATFORM=PLATFORM_DESKTOP CC="$CC"
    fi

    INCLUDE_DIRS="$INCLUDE_DIRS -I$RAYLIB_SUBPROJECT_DIR/src"
    LDLIBS="$RAYLIB_STATIC_LIB -lGL -lX11 -lXrandr -lXinerama -lXi -lXcursor -lpthread -ldl -lrt -lm"
else
    if command -v "$PKG_CONFIG_BIN" >/dev/null 2>&1; then
        if $PKG_CONFIG_BIN --exists raylib; then
            RAYLIB_CFLAGS="$($PKG_CONFIG_BIN --cflags raylib)"
            RAYLIB_LIBS="$($PKG_CONFIG_BIN --libs raylib)"
            CFLAGS="$CFLAGS $RAYLIB_CFLAGS"
            LDLIBS="$LDLIBS $RAYLIB_LIBS"
        else
            LDLIBS="$LDLIBS -lraylib"
        fi
    else
        LDLIBS="$LDLIBS -lraylib"
    fi
fi

SRC_FILES=("./src/unity_build.c")
INCLUDE_DIRS="-I./include -I./vendor"
OUTPUT_DIR="./build"
EXECUTABLE_NAME="galaxy_visualization_raylib"

if [ "$EMBED_ASSETS" = true ]; then
    OUTPUT_DIR="./build_embedded"
    CFLAGS="$CFLAGS -DEMBED_ASSETS"
    INCLUDE_DIRS="$INCLUDE_DIRS -I$OUTPUT_DIR"
fi

if [ "$MINGW" = true ]; then
    if [ "$RUN" = true ]; then
        echo "[WARN] Cannot run Windows .exe on Linux. Skipping run."
        RUN=false
    fi

    EMBED_ASSETS=true
    STATIC_RAYLIB=true
    OUTPUT_DIR="./build_mingw_embedded_static"

    echo "=== Cross-compiling Windows (mingw64) ==="
    if [ -d "./subprojects/raylib" ]; then
        rm -rf ./subprojects/raylib
    fi
    meson subprojects download raylib
    meson setup --reconfigure "$OUTPUT_DIR" \
        --cross-file ./cross/mingw64.ini \
        -Dembed_assets=true \
        -Draylib_static=true
    meson compile -C "$OUTPUT_DIR"
    echo ""
    echo "=== Build complete! ==="
    ls -lh "$OUTPUT_DIR/galaxy_visualization_raylib.exe"
    exit 0
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
