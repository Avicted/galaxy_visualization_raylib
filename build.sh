#!/bin/bash

set -xe

[ -d ./build ] && rm -rf ./build
mkdir -p ./build

CC=clang
CFLAGS="-std=c99 -ggdb -O0 -Wall -Wextra -Werror"
LDLIBS="-lm -lraylib -lfftw3 -lpthread"

SRC_FILES=(
  "./src/main.c"
)
INCLUDE_DIRS="-I./include"
OUTPUT_DIR="./build"
EXECUTABLE_NAME="galaxy_visualization_raylib"

cp -r resources $OUTPUT_DIR

$CC $CFLAGS $INCLUDE_DIRS -o $OUTPUT_DIR/$EXECUTABLE_NAME ${SRC_FILES[@]} $LDLIBS
