#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
GIMP_PLUGINS="${NANOTANK_GIMP_PLUGINS_DIR:-$HOME/.var/app/org.gimp.GIMP/config/GIMP/3.2/plug-ins}"
GEGL_PLUGINS="${NANOTANK_GEGL_PLUGINS_DIR:-$HOME/.var/app/org.gimp.GIMP/data/gegl-0.4/plug-ins}"
OUT="$ROOT/output"
if [[ ! -d "$OUT/gimp-plug-ins" ]]; then
  printf 'Noch keine Ausgabe vorhanden. Zuerst: bash scripts/build.sh\n' >&2
  exit 1
fi

mkdir -p -- "$GIMP_PLUGINS" "$GEGL_PLUGINS"
link_safely() {
  local source="$1" dest="$2"
  if [[ -L "$dest" ]] && [[ "$(readlink -- "$dest")" == "$source" ]]; then
    printf 'Bereits verlinkt: %s\n' "$dest"
  elif [[ -e "$dest" || -L "$dest" ]]; then
    printf 'KONFLIKT (nicht überschrieben): %s\n' "$dest" >&2
    return 1
  else
    ln -s -- "$source" "$dest"
    printf 'Verlinkt: %s -> %s\n' "$dest" "$source"
  fi
}

shopt -s nullglob
for source in "$OUT/gimp-plug-ins"/nanotank-*; do
  [[ -d "$source" ]] || continue
  name="$(basename -- "$source")"
  if [[ ! -x "$source/$name.py" && ! -x "$source/$name" ]]; then
    printf 'Übersprungen, kein ausführbarer Einstiegspunkt: %s\n' "$source" >&2
    continue
  fi
  link_safely "$source" "$GIMP_PLUGINS/$name"
done
for source in "$OUT/gegl"/nanotank-*.so; do
  [[ -f "$source" ]] || continue
  link_safely "$source" "$GEGL_PLUGINS/$(basename -- "$source")"
done

printf '\nErledigt. GIMP komplett beenden und neu starten.\n'
printf 'Systemtest: Bild öffnen -> Filter -> NanoTank -> Systemtest.\n'
