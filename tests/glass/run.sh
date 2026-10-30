#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
flatpak run --devel --filesystem="$PWD" --command=sh org.gimp.GIMP -c '
set -eu
cd "$1"
export PKG_CONFIG_PATH=/app/lib/pkgconfig
cc -std=c11 -O2 -Wall -Wextra tests/glass/render.c -o output/glass-render $(pkg-config --cflags --libs gegl-0.4) -lm
GEGL_PATH="$PWD/output/gegl:/app/lib/gegl-0.4" output/glass-render
' sh "$PWD"
