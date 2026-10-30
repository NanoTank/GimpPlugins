#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Build native GEGL module separately from common Python build.sh.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
cd "$ROOT"
if ! command -v pkg-config >/dev/null || ! pkg-config --exists gegl-0.4; then
  echo 'GEGL-Entwicklungsdateien fehlen (pkg-config gegl-0.4).' >&2
  echo 'Mit der zu GIMP Flatpak 3.2.6 passenden SDK/GEGL-Laufzeit bauen.' >&2
  exit 1
fi
GEGL_VERSION="$(pkg-config --modversion gegl-0.4)"
if [[ "$GEGL_VERSION" != '0.4.72' && "${NANOTANK_ALLOW_OTHER_GEGL:-0}" != '1' ]]; then
  echo "GEGL $GEGL_VERSION gefunden; Ziel GIMP Flatpak benutzt 0.4.72." >&2
  echo 'Abbruch zur Vermeidung eines ABI-Mismatch. Nur nach ABI-Prüfung:' >&2
  echo 'NANOTANK_ALLOW_OTHER_GEGL=1 bash scripts/build-glass.sh' >&2
  exit 1
fi
mkdir -p output/gegl
cc -std=c11 -O2 -fPIC -shared -Wall -Wextra \
  $(pkg-config --cflags gegl-0.4) \
  plugins/nanotank-glass/nanotank-glass.c \
  -o output/gegl/nanotank-glass.so \
  $(pkg-config --libs gegl-0.4) -lm
cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic \
  tests/test-glass-core.c -o /tmp/nanotank-glass-core-test -lm
/tmp/nanotank-glass-core-test
rm -f /tmp/nanotank-glass-core-test
printf 'Gebaut mit GEGL %s: output/gegl/nanotank-glass.so\n' "$GEGL_VERSION"
echo 'Danach: bash scripts/install-flatpak.sh; GIMP vollständig neu starten.'
