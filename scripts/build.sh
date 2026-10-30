#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
SRC="$ROOT/plugins"
OUT="$ROOT/output/gimp-plug-ins"
mkdir -p -- "$OUT" "$ROOT/output/gegl"

shopt -s nullglob
count=0
for source in "$SRC"/nanotank-*; do
  [[ -d "$source" ]] || continue
  name="$(basename -- "$source")"
  entry="$source/$name.py"
  if [[ ! -f "$entry" ]]; then
    printf 'FEHLER: %s fehlt (Python-Einstiegspunkt).\n' "$entry" >&2
    exit 1
  fi
  target="$OUT/$name"
  if [[ -L "$target" ]]; then
    printf 'FEHLER: Ausgabeziel ist selbst ein Symlink: %s\n' "$target" >&2
    exit 1
  fi
  rm -rf -- "$target"
  cp -R -- "$source" "$target"
  chmod u+x -- "$target/$name.py"
  python3 - "$target/$name.py" <<'PY'
import ast
import pathlib
import sys
ast.parse(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
PY
  printf 'Gebaut: %s\n' "$name"
  count=$((count + 1))
done
printf 'Fertig: %d Python-Plug-in(s) in output/gimp-plug-ins/.\n' "$count"
printf 'GEGL-Binärdateien (*.so) kommen später nach output/gegl/.\n'
