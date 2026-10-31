#!/usr/bin/env bash
# SPDX-License-Identifier: MIT – NanoTank
# Uses the common SDK build path; no alternate compiler or installed modules.
set -euo pipefail
cd "$(dirname "$0")/../.."
mkdir -p output/glass-p141/p14-source
git archive 9c77567 gegl/nanotank-glass scripts/build-gegl.sh |
  tar -x -C output/glass-p141/p14-source
for module in nanotank-glass nanotank-glass-refraction; do
  if [[ ! -f "output/glass-p141/p14-gegl/$module.so" ]]; then
    NANOTANK_GEGL_BUILD_DIR="$PWD/output/glass-p141/p14-gegl" \
      bash output/glass-p141/p14-source/scripts/build-gegl.sh "$module"
  fi
  NANOTANK_GEGL_BUILD_DIR="$PWD/output/glass-p141/gegl" \
    bash scripts/build-gegl.sh "$module"
done
for version in p14 p141; do
  modules="$PWD/output/glass-p141/gegl"
  [[ "$version" != p14 ]] || modules="$PWD/output/glass-p141/p14-gegl"
  NANOTANK_GLASS_TEST_MODULES="$modules" \
  NANOTANK_GLASS_GALLERY_ROOT="$PWD/output/glass-p141/$version" \
    bash tests/glass/run-p14.sh
done
python3 tests/glass/gallery-p141.py
