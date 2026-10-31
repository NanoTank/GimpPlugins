#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Uses the historical common SDK build script, no second build implementation.
set -euo pipefail
cd "$(dirname "$0")/../.."
revision=320707d
root="$PWD/output/glass-p11"
label=p1
module_dir="$root/baseline-source/output/gegl"
if [[ "${1:-}" == --p11 ]]; then
  revision=451f592
  root="$PWD/output/glass-p12"
  label=p11
  module_dir="$root/baseline-module"
fi
mkdir -p "$root/baseline-source"
git archive "$revision" gegl/nanotank-glass scripts/build-gegl.sh |
  tar -x -C "$root/baseline-source"
NANOTANK_GEGL_BUILD_DIR="$module_dir" \
  bash "$root/baseline-source/scripts/build-gegl.sh" nanotank-glass
NANOTANK_GLASS_GALLERY_ROOT="$root" NANOTANK_GLASS_BASELINE_LABEL="$label" \
NANOTANK_GLASS_TEST_MODULES="$module_dir" \
  bash tests/glass/run.sh --baseline
