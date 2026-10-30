#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Diagnostik ohne Änderungen an der GIMP-Installation.
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
PLUGINS="${NANOTANK_GIMP_PLUGINS_DIR:-$HOME/.var/app/org.gimp.GIMP/config/GIMP/3.2/plug-ins}"
ENTRY="$PLUGINS/nanotank-systemtest/nanotank-systemtest.py"
SOURCE="$ROOT/output/gimp-plug-ins/nanotank-systemtest/nanotank-systemtest.py"

printf 'Projekt: %s\n' "$ROOT"
printf 'Erzeugt: %s\n' "$SOURCE"
if [[ -f "$SOURCE" && -x "$SOURCE" ]]; then
  printf 'OK: gebautes Plug-in existiert und ist ausführbar\n'
else
  printf 'FEHLT: gebautes Plug-in. Zuerst bash scripts/build.sh ausführen\n'
fi

printf 'Installiert: %s\n' "$ENTRY"
if [[ -L "$PLUGINS/nanotank-systemtest" ]]; then
  printf 'Symlink-Ziel: %s\n' "$(readlink -- "$PLUGINS/nanotank-systemtest")"
else
  printf 'Kein Verzeichnis-Symlink für Systemtest vorhanden\n'
fi
if [[ -f "$ENTRY" && -x "$ENTRY" ]]; then
  printf 'OK: Installation über Host erreichbar und ausführbar\n'
else
  printf 'FEHLT: über Host nicht ausführbar/erreichbar\n'
fi

if command -v flatpak >/dev/null 2>&1; then
  printf '\nPrüfung innerhalb der GIMP-Flatpak-Sandbox:\n'
  flatpak run --command=sh org.gimp.GIMP -c '
    p="$XDG_CONFIG_HOME/GIMP/3.2/plug-ins/nanotank-systemtest/nanotank-systemtest.py"
    printf "XDG_CONFIG_HOME=%s\n" "$XDG_CONFIG_HOME"
    if [ -f "$p" ] && [ -x "$p" ]; then
      printf "OK: Systemtest in Sandbox erreichbar und ausführbar\n"
    else
      printf "FEHLT: Systemtest in Sandbox nicht ausführbar/erreichbar: %s\n" "$p"
    fi
  '
fi

printf '\nGIMP: Bearbeiten → Einstellungen → Ordner → Plug-ins prüfen.\n'
