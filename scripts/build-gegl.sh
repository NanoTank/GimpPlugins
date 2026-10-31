#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Baut GEGL-Testmodul mit dem SDK der *installierten GIMP-Flatpak-App*.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
# Ohne Argument bleibt der bisherige GEGL-Systemtest das Build-Ziel.
# Neue Module werden gezielt gebaut, nicht unbemerkt zusammen mit dem Test.
MODULE="${1:-nanotank-gegl-test}"
case "$MODULE" in
  nanotank-gegl-test|nanotank-glass|nanotank-glass-refraction) ;;
  *) echo "Unbekanntes NanoTank-GEGL-Modul: $MODULE" >&2; exit 2 ;;
esac
SRC="$ROOT/gegl/$MODULE/$MODULE.c"
if [[ "$MODULE" == nanotank-glass-refraction ]]; then
  SRC="$ROOT/gegl/nanotank-glass/$MODULE.c"
fi
# Isoliertes Testziel, ohne ein eventuell geladenes Installationsmodul zu ersetzen.
BUILD_DIR="${NANOTANK_GEGL_BUILD_DIR:-$ROOT/output/gegl}"
OUT="$BUILD_DIR/$MODULE.so"

command -v flatpak >/dev/null || { echo "FEHLER: flatpak fehlt" >&2; exit 1; }
[[ -f "$SRC" ]] || { echo "FEHLER: Quelle fehlt: $SRC" >&2; exit 1; }
if [[ "$MODULE" == nanotank-glass* && "$BUILD_DIR" == "$ROOT/output/gegl" ]]; then
  gimp_instance_running() {
    flatpak ps --columns=application 2>/dev/null | grep -Fxq org.gimp.GIMP
  }
  if gimp_instance_running; then
    # A preceding targeted SDK build can briefly remain registered.
    for attempt in 1 2 3 4 5 6 7 8 9 10; do
      sleep 0.3
      if ! gimp_instance_running; then break; fi
    done
  fi
  if gimp_instance_running; then
    echo "FEHLER: GIMP vollständig schließen, bevor das Glass-Buildziel ersetzt wird." >&2
    exit 1
  fi
fi
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

  # Die Entwicklungsdateien von GIMP liegen im /app-Praefix.
  # Innerhalb von flatpak run --devel ist dieser Pfad nicht immer im
  # pkg-config-Suchpfad. Zuerst die echte Datei suchen statt zu raten.
  pcdir=""
  for dir in /app/lib/pkgconfig /app/lib64/pkgconfig /app/lib/*/pkgconfig /app/share/pkgconfig; do
    if [ -f "$dir/gegl-0.4.pc" ]; then
      pcdir="$dir"
      break
    fi
  done
  if [ -z "$pcdir" ]; then
    echo "FEHLER: gegl-0.4.pc wurde im GIMP-Flatpak nicht gefunden." >&2
    echo "Moegliche Dateien unter /app:" >&2
    find /app -name gegl-0.4.pc -o -name gegl-op.h 2>/dev/null | head -20 >&2 || true
    echo "Bitte bash scripts/diagnose-gegl-sdk.sh ausfuehren." >&2
    exit 1
  fi
  export PKG_CONFIG_PATH="$pcdir${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
  if ! pkg-config --exists gegl-0.4; then
    echo "FEHLER: gegl-0.4.pc existiert, aber Abhaengigkeiten fehlen:" >&2
    pkg-config --print-errors --exists gegl-0.4 >&2 || true
    exit 1
  fi
  header=""
  for h in /app/include/gegl-0.4/operation/gegl-op.h /app/include/gegl-0.4/gegl-op.h; do
    if [ -f "$h" ]; then header="$h"; break; fi
  done
  if [ -z "$header" ]; then
    header="$(find /app/include -name gegl-op.h -print -quit 2>/dev/null || true)"
  fi
  if [ -z "$header" ]; then
    echo "FEHLER: gegl-op.h fehlt im GIMP-SDK." >&2
    echo "Bitte bash scripts/diagnose-gegl-sdk.sh ausfuehren." >&2
    exit 1
  fi
  printf "GEGL Buildversion: "
  pkg-config --modversion gegl-0.4
  printf "SDK-Pfad: %s, Operationsheader: %s\n" "$pcdir" "$header"
  # gegl-op.h inkludiert GEGL_OP_C_SOURCE nochmals: auch das eigene
  # Quellverzeichnis muss deshalb im Include-Suchpfad stehen.
  # -isystem vermindert unnoetige Warnungen aus GEGL-SDK-Headers.
  # pkg-config liefert eine Liste einzelner Compiler-/Linker-Flags.
  # In diesem eng kontrollierten SDK sind die Flags ohne Leerzeichen.
  # shellcheck disable=SC2086
  cc -std=c11 -O2 -fPIC -shared -Wall -Wextra \
     -I"$(dirname -- "$src")" \
     -isystem "$(dirname -- "$header")" \
     $(pkg-config --cflags gegl-0.4) "$src" \
     -o "$out.tmp" $(pkg-config --libs gegl-0.4) -lm
  mv -f -- "$out.tmp" "$out"
  test -s "$out"
  printf "GEGL-Modul gebaut: %s\n" "$out"
' sh "$SRC" "$OUT"

if [[ "$BUILD_DIR" == "$ROOT/output/gegl" ]]; then
  printf '\nBuild erfolgreich. GIMP geschlossen lassen, dann:\n'
  printf 'bash scripts/deploy-gegl-flatpak.sh %s && bash scripts/diagnose-gegl-flatpak.sh %s\n' "$MODULE" "$MODULE"
else
  printf '\nIsolierter Testbuild: %s (installiertes Modul unverändert).\n' "$OUT"
fi
