#!/bin/bash

# generate_embedded.sh - Generate compressed C headers from assets
# Usage: ./scripts/generate_embedded.sh <output_dir>

set -e

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

OUTPUT_DIR="${1:-build/embedded}"
mkdir -p "$OUTPUT_DIR"

# Change to project root for relative paths
cd "$PROJECT_ROOT"

# Function to compress a file and generate C header
generate_compressed_header() {
    local input_file="$1"
    local var_name="$2"
    local output_file="$OUTPUT_DIR/${var_name}.h"
    
    if [ ! -f "$input_file" ]; then
        echo "Warning: $input_file not found, skipping"
        return
    fi
    
    local original_size=$(stat -c%s "$input_file")
    
    # Compress with zlib format (not gzip) using python for compatibility
    local compressed_file=$(mktemp)
    python3 -c "
import zlib
import sys
with open('$input_file', 'rb') as f:
    data = f.read()
compressed = zlib.compress(data, 9)
sys.stdout.buffer.write(compressed)
" > "$compressed_file"
    local compressed_size=$(stat -c%s "$compressed_file")
    
    # Create proper header with size constants
    cat > "$output_file" << EOF
// Auto-generated from: $input_file
// Original size: $original_size bytes
// Compressed size: $compressed_size bytes
#ifndef EMBEDDED_${var_name^^}_H
#define EMBEDDED_${var_name^^}_H

#include <stddef.h>

static const size_t ${var_name}_original_size = ${original_size};
static const size_t ${var_name}_compressed_size = ${compressed_size};

static const unsigned char ${var_name}_data[] = {
EOF

    # Convert compressed file to C array (xxd without -i to get just the hex, then format it)
    xxd -i < "$compressed_file" >> "$output_file"
    
    echo "};" >> "$output_file"
    echo "" >> "$output_file"
    echo "#endif // EMBEDDED_${var_name^^}_H" >> "$output_file"
    rm "$compressed_file"
    
    echo "Generated: $output_file ($original_size -> $compressed_size bytes, $(echo "scale=1; $compressed_size * 100 / $original_size" | bc)%)"
}

# Function for text files that benefit more from compression
generate_text_header() {
    generate_compressed_header "$1" "$2"
}

echo "Generating embedded asset headers..."
echo "Output directory: $OUTPUT_DIR"
echo ""

# Shaders (small, store as plain text for easier debugging)
echo "=== Shaders (uncompressed) ==="
for shader in shaders/*.vs shaders/*.fs; do
    if [ -f "$shader" ]; then
        base=$(basename "$shader" | tr '.' '_')
        var_name="shader_${base}"
        output_file="$OUTPUT_DIR/${var_name}.h"
        
        size=$(stat -c%s "$shader")
        
        cat > "$output_file" << EOF
// Auto-generated from: $shader
#ifndef EMBEDDED_${var_name^^}_H
#define EMBEDDED_${var_name^^}_H

static const char ${var_name}_data[] =
EOF
        # Convert file to C string literal
        sed 's/\\/\\\\/g; s/"/\\"/g; s/^/    "/; s/$/\\n"/' "$shader" >> "$output_file"
        echo ";" >> "$output_file"
        echo "" >> "$output_file"
        echo "#endif // EMBEDDED_${var_name^^}_H" >> "$output_file"
        
        echo "Generated: $output_file ($size bytes, uncompressed string)"
    fi
done

echo ""
echo "=== Binary Assets (compressed) ==="

# Font
generate_compressed_header "assets/fonts/Perfect DOS VGA 437.ttf" "font_perfect_dos"

# Icon
generate_compressed_header "assets/images/app_icon.png" "icon_app"

# Earth model
generate_compressed_header "assets/Earth_1_12756_optimized.glb" "model_earth"

echo ""
echo "=== Data Files (compressed) ==="

# Data files
generate_compressed_header "input_data/data_100k_arcmin.txt" "data_arcmin_a"
generate_compressed_header "input_data/flat_100k_arcmin.txt" "data_arcmin_b"
generate_compressed_header "input_data/redshift_input_data/seyfert.dat" "data_seyfert"
generate_compressed_header "input_data/redshift_input_data/saga-dr3-satellites.txt" "data_saga_dr3"

echo ""
echo "=== Summary ==="
total_size=$(du -sb "$OUTPUT_DIR" | cut -f1)
echo "Total embedded headers size: $(numfmt --to=iec $total_size)"
echo ""
echo "Done! Headers generated in $OUTPUT_DIR"
