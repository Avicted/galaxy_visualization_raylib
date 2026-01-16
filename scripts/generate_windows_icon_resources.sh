#!/usr/bin/env bash

set -euo pipefail

if [[ $# -lt 3 ]]; then
  echo "Usage: $0 <input_png> <output_dir> <magick_path>" >&2
  exit 1
fi

input_png="$1"
output_dir="$2"
magick_bin="$3"

mkdir -p "$output_dir"

"$magick_bin" "$input_png" -define icon:auto-resize=256,128,64,48,32,16 "$output_dir/app_icon.ico"

cat > "$output_dir/app.rc" <<'EOF'
APP_ICON ICON "app_icon.ico"
EOF
