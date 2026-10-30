#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Baut GEGL-Testmodul mit dem SDK der *installierten GIMP-Flatpak-App*.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
SRC="$ROOT/gegl/nanotank-gegl-test/nanotank-gegl-test.c"
OUT="$ROOT/output/gegl/nanotank-gegl-test.so"

command -v flatpak >/dev/null || { echo "FEHLER: flatpak fehlt" >&2; exit 1; }
[[ -f "$SRC" ]] || { echo "FEHLER: Quelle fehlt: $SRC" >&2; exit 1; }
mkdir -p -- "$(dirname -- "$OUT")"

# --devel wechselt die App-Laufzeit auf das zu GIMP passende GNOME-SDK.
# /app in dieser Sandbox enthaelt GIMPs eigenes SDK mit GEGL-Headers/Libs.
flatpak run --devel --command=sh org.gimp.GIMP -c '
  set -eu
  src="$1"
  out="$2"
  command -v cc >/dev/null 2>&1 || {
    echo "FEHLER: Kein C-Compiler im Flatpak-SDK" >&2
    exit 1
  }
  command -v pkg-config >/dev/null 2>&1 || {
    echo "FEHLER: pkg-config fehlt im Flatpak-SDK" >&2
    exit 1
  }
  pkg-config --exists gegl-0.4 || {
    echo "FEHLER: gegl-0.4.pc fehlt im GIMP-SDK; keine Host-Bibliothek benutzen." >&2
    exit 1
  }
  printf "GEGL Buildversion: "
  pkg-config --modversion gegl-0.4
  # pkg-config liefert eine Liste einzelner Compiler-/Linker-Flags.
  # In diesem eng kontrollierten SDK sind die Flags ohne Leerzeichen.
  # shellcheck disable=SC2086
  cc -std=c11 -O2 -fPIC -shared -Wall -Wextra \
     $(pkg-config --cflags gegl-0.4) "$src" \
     -o "$out.tmp" $(pkg-config --libs gegl-0.4)
  mv -f -- "$out.tmp" "$out"
  test -s "$out"
  printf "GEGL-Modul gebaut: %s\n" "$out"
' sh "$SRC" "$OUT"

printf '\nNächster Schritt (GIMP vorher schließen): bash scripts/deploy-gegl-flatpak.sh\n'
