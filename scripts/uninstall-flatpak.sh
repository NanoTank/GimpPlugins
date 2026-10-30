#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Entfernt NUR Symlinks, die auf dieses Repository zeigen.
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
GIMP_PLUGINS="${NANOTANK_GIMP_PLUGINS_DIR:-$HOME/.var/app/org.gimp.GIMP/config/GIMP/3.2/plug-ins}"
GEGL_PLUGINS="${NANOTANK_GEGL_PLUGINS_DIR:-$HOME/.var/app/org.gimp.GIMP/data/gegl-0.4/plug-ins}"

shopt -s nullglob
for item in "$GIMP_PLUGINS"/nanotank-*; do
  [[ -L "$item" ]] || continue
  target="$(readlink -- "$item")"
  if [[ "$target" == "$ROOT/output/gimp-plug-ins/"* ]]; then
    rm -- "$item"
    printf 'Link entfernt: %s\n' "$item"
  fi
done
for item in "$GEGL_PLUGINS"/nanotank-*.so; do
  [[ -L "$item" ]] || continue
  target="$(readlink -- "$item")"
  if [[ "$target" == "$ROOT/output/gegl/"* ]]; then
    rm -- "$item"
    printf 'Link entfernt: %s\n' "$item"
  fi
done
printf 'Fertig. GIMP nach dem Entfernen neu starten.\n'
