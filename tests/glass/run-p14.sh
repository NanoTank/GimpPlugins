#!/usr/bin/env bash
# SPDX-License-Identifier: MIT – NanoTank
set -euo pipefail
cd "$(dirname "$0")/../.."
mkdir -p output/glass-p14
flatpak run --devel --filesystem="$PWD" --command=sh org.gimp.GIMP -c '
set -eu
cd "$1"
export PKG_CONFIG_PATH=/app/lib/pkgconfig
cc -std=c11 -O2 -Wall -Wextra tests/glass/refraction.c -o output/glass-p14/refraction-test $(pkg-config --cflags --libs gegl-0.4) -lm
GEGL_PATH="${NANOTANK_GLASS_TEST_MODULES:-$PWD/output/glass-p14/gegl}:/app/lib/gegl-0.4" output/glass-p14/refraction-test
' sh "$PWD"
