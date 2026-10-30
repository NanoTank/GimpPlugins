#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Entfernt ausschließlich Symlinks/Kopien, deren Ursprung dieses Repository ist.
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
GIMP_PLUGINS="${NANOTANK_GIMP_PLUGINS_DIR:-$HOME/.var/app/org.gimp.GIMP/config/GIMP/3.2/plug-ins}"
GEGL_PLUGINS="${NANOTANK_GEGL_PLUGINS_DIR:-$HOME/.var/app/org.gimp.GIMP/data/gegl-0.4/plug-ins}"
shopt -s nullglob

for item in "$GIMP_PLUGINS"/nanotank-*; do
  if [[ -L "$item" && "$(readlink -- "$item")" == "$ROOT/output/gimp-plug-ins/$(basename -- "$item")" ]]; then
    rm -- "$item"
    printf 'NanoTank-Link entfernt: %s\n' "$item"
  elif [[ -d "$item" && ! -L "$item" && -f "$item/.nanotank-managed-origin" &&
          "$(cat -- "$item/.nanotank-managed-origin")" == "$ROOT/output/gimp-plug-ins/$(basename -- "$item")" ]]; then
    rm -rf -- "$item"
    printf 'NanoTank-Kopie entfernt: %s\n' "$item"
  fi
done
for item in "$GEGL_PLUGINS"/nanotank-*.so; do
  expected="$ROOT/output/gegl/$(basename -- "$item")"
  marker="$(dirname -- "$item")/.nanotank-$(basename -- "$item").origin"
  if [[ -L "$item" && "$(readlink -- "$item")" == "$expected" ]]; then
    rm -- "$item"
    printf 'GEGL-Link entfernt: %s\n' "$item"
  elif [[ -f "$item" && ! -L "$item" && -f "$marker" && "$(cat -- "$marker")" == "$expected" ]]; then
    rm -- "$item" "$marker"
    printf 'GEGL-Kopie entfernt: %s\n' "$item"
  fi
done
printf 'Fertig. GIMP neu starten.\n'
