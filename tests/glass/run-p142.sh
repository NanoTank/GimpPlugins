#!/usr/bin/env bash
# SPDX-License-Identifier: MIT — NanoTank
# Focused checks; --benchmark additionally rebuilds the immutable reference and
# measures GEGL plus real GIMP layer filters (not an interactive dialog test).
set -euo pipefail
cd "$(dirname "$0")/../.."
mkdir -p output/glass-p142
flatpak run --devel --filesystem="$PWD" --command=sh org.gimp.GIMP -c '
set -eu
export PKG_CONFIG_PATH=/app/lib/pkgconfig
cc -O2 -Wall -Wextra tests/glass/geometry-cache.c -o output/glass-p142/geometry-cache $(pkg-config --cflags --libs gegl-0.4) -lm
output/glass-p142/geometry-cache
'
if [[ "${1:-}" != --benchmark ]]; then exit 0; fi
mkdir -p output/glass-p142/baseline-source
git archive bc5ad2c gegl/nanotank-glass scripts/build-gegl.sh | tar -x -C output/glass-p142/baseline-source
for tag in baseline optimized; do
  if [[ "$tag" == baseline ]]; then
    source_dir="$PWD/output/glass-p142/baseline-source"
  else
    source_dir="$PWD"
  fi
  module_dir="$PWD/output/glass-p142/$tag-gegl"
  NANOTANK_GEGL_BUILD_DIR="$module_dir" bash "$source_dir/scripts/build-gegl.sh" nanotank-glass
  flatpak run --devel --filesystem="$PWD" --command=sh org.gimp.GIMP -c '
set -eu
export PKG_CONFIG_PATH=/app/lib/pkgconfig
cc -O2 -Wall -Wextra "-DNT_GEOMETRY_HEADER=\"$1/gegl/nanotank-glass/glass-geometry.h\"" tests/glass/performance.c -o output/glass-p142/performance $(pkg-config --cflags --libs gegl-0.4) -lm
GEGL_PATH="$2:/app/lib/gegl-0.4" output/glass-p142/performance "$3" > "output/glass-p142/$3.csv"
' sh "$source_dir" "$module_dir" "$tag"
  flatpak run --env=GIMP3_DIRECTORY="$PWD/output/glass-p142/gimp-profile" \
    --env=NANOTANK_GLASS_ROOT="$PWD" --env=NANOTANK_PERF_TAG="$tag" \
    --env=GEGL_PATH="$module_dir:/app/lib/gegl-0.4" \
    --command=gimp-console org.gimp.GIMP --new-instance --no-interface --no-data \
    --no-fonts --no-splash --console-messages --batch-interpreter=python-fu-eval \
    --batch 'import os; exec(open(os.environ["NANOTANK_GLASS_ROOT"]+"/tests/glass/gimp-performance.py").read())' \
    --quit > "output/glass-p142/gimp-$tag.log" 2>&1
  # GIMP can exit successfully after a Python batch exception.
  test -f "output/glass-p142/gimp-$tag.json"
  grep -q '^GIMP_PERFORMANCE_OK' "output/glass-p142/gimp-$tag.log"
done
for reference in output/glass-p142/baseline-*.bin; do
  cmp "$reference" "${reference/baseline-/optimized-}"
done
printf '%s\n' 'PASS: 35 actual GEGL baseline/optimized renders are bit-identical; GIMP benchmarks completed.'
