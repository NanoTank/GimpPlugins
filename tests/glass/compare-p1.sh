#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Uses the historical common SDK build script, no second build implementation.
set -euo pipefail
cd "$(dirname "$0")/../.."
mkdir -p output/glass-p11/baseline-source
# Fixed P1 source revision makes the comparison independent of installed modules.
git archive 320707d gegl/nanotank-glass scripts/build-gegl.sh |
  tar -x -C output/glass-p11/baseline-source
bash output/glass-p11/baseline-source/scripts/build-gegl.sh nanotank-glass
NANOTANK_GLASS_TEST_MODULES="$PWD/output/glass-p11/baseline-source/output/gegl" \
  bash tests/glass/run.sh --baseline
