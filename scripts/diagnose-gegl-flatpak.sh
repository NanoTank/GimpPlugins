#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
flatpak run --command=sh org.gimp.GIMP -c '
  set -eu
  source="$1"
  dest="${XDG_DATA_HOME:?}/gegl-0.4/plug-ins/nanotank-gegl-test.so"
  printf "XDG_DATA_HOME=%s\n" "$XDG_DATA_HOME"
  for path in "$source" "$dest"; do
    if [ -r "$path" ]; then
      printf "LESBAR: %s\n" "$path"
    else
      printf "FEHLT / NICHT LESBAR: %s\n" "$path"
    fi
  done
  if [ -L "$dest" ]; then
    printf "Symlink-Ziel: %s\n" "$(readlink "$dest")"
  fi
  if [ -r "$dest" ]; then
    echo "Sichtbarkeit bestanden; GEGL-Registrierung noch in GIMP zu prüfen."
  else
    echo "Kein lesbares GEGL-Modul installiert." >&2
    exit 1
  fi
' sh "$ROOT/output/gegl/nanotank-gegl-test.so"
