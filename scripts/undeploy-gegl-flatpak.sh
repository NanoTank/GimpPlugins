#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Entfernt ausschließlich GEGL-Symlinks oder markierte Kopien dieses Klons.
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
MODULE="${1:-all}"
case "$MODULE" in
  all|nanotank-gegl-test|nanotank-glass) ;;
  *) echo "Unbekanntes NanoTank-Modul: $MODULE" >&2; exit 2 ;;
esac
if flatpak ps --columns=application 2>/dev/null | grep -Fxq 'org.gimp.GIMP'; then
  echo "Bitte GIMP vor der Entfernung vollständig schließen." >&2
  exit 1
fi
flatpak run --command=sh org.gimp.GIMP -c '
  set -eu
  root="$1"
  wanted="$2"
  dest_dir="${XDG_DATA_HOME:?}/gegl-0.4/plug-ins"
  [ -d "$dest_dir" ] || exit 0
  for dest in "$dest_dir"/nanotank-*.so; do
    [ -e "$dest" ] || [ -L "$dest" ] || continue
    name="${dest##*/}"
    [ "$wanted" = all ] || [ "$name" = "$wanted.so" ] || continue
    origin="$root/output/gegl/$name"
    marker="$dest.nanotank-origin"
    if [ -L "$dest" ] && [ "$(readlink "$dest")" = "$origin" ]; then
      rm -- "$dest"
      echo "GEGL-Symlink entfernt: $dest"
    elif [ -f "$dest" ] && [ ! -L "$dest" ] &&
         [ -f "$marker" ] && [ "$(cat "$marker")" = "$origin" ]; then
      rm -- "$dest" "$marker"
      echo "Verwaltete GEGL-Kopie entfernt: $dest"
    fi
  done
' sh "$ROOT" "$MODULE"

echo "Fertig. Quellen und output/ bleiben erhalten."
