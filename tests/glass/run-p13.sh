#!/usr/bin/env bash
# SPDX-License-Identifier: MIT – NanoTank
set -euo pipefail
cd "$(dirname "$0")/../.."
mkdir -p output/glass-p13
if [[ "${1:-}" == --compare-p12 ]]; then
  mkdir -p output/glass-p13/p12-source
  git archive eba9090 gegl/nanotank-glass scripts/build-gegl.sh |
    tar -x -C output/glass-p13/p12-source
  NANOTANK_GEGL_BUILD_DIR="$PWD/output/glass-p13/p12-gegl" \
    bash output/glass-p13/p12-source/scripts/build-gegl.sh nanotank-glass
fi
flatpak run --devel --filesystem="$PWD" --command=sh org.gimp.GIMP -c '
set -eu
cd "$1"
export PKG_CONFIG_PATH=/app/lib/pkgconfig
cc -std=c11 -O2 -Wall -Wextra tests/glass/geometry.c -lm -o output/glass-p13/geometry-test
output/glass-p13/geometry-test
cc -std=c11 -O2 -Wall -Wextra tests/glass/geometry-render.c -o output/glass-p13/geometry-render $(pkg-config --cflags --libs gegl-0.4) -lm
GEGL_PATH="$PWD/output/glass-p13/gegl:/app/lib/gegl-0.4" output/glass-p13/geometry-render
if [ "$2" = --compare-p12 ]; then
  GEGL_PATH="$PWD/output/glass-p13/p12-gegl:/app/lib/gegl-0.4" output/glass-p13/geometry-render --baseline
  cmp output/glass-p13/legacy.bin output/glass-p13/baseline.bin
  echo "PASS: legacy mode is pixel-identical to the actual P1.2 module"
fi
' sh "$PWD" "${1:-}"
