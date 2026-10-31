#!/usr/bin/env bash
# SPDX-License-Identifier: MIT – NanoTank
set -euo pipefail
cd "$(dirname "$0")/../.."
mkdir -p output/glass-p14
if ! flatpak run --env=GIMP3_DIRECTORY="$PWD/output/glass-p14/probe-profile" \
  --env=NANOTANK_GLASS_ROOT="$PWD" \
  --env=GEGL_PATH="${NANOTANK_GLASS_TEST_MODULES:-$PWD/output/glass-p14/gegl}:/app/lib/gegl-0.4" \
  --command=gimp-console org.gimp.GIMP --new-instance --no-interface --no-data \
  --no-fonts --no-splash --console-messages --batch-interpreter=python-fu-eval \
  --batch 'import os; exec(open(os.environ["NANOTANK_GLASS_ROOT"]+"/tests/glass/gimp-background.py").read())' \
  --quit > output/glass-p14/gimp-batch.log 2>&1; then
  tail -n 40 output/glass-p14/gimp-batch.log
  exit 1
fi
grep -E 'REGISTERED_PROCEDURE|PLUGIN_REGISTRATION|NATIVE_PIXEL|HELPER_OK|FEEDBACK_REFUSED|XCF_SNAPSHOT|batch command executed' output/glass-p14/gimp-batch.log
