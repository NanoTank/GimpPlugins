#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Diagnostik ohne Änderungen an der GIMP-Installation.
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
PLUGINS="${NANOTANK_GIMP_PLUGINS_DIR:-$HOME/.var/app/org.gimp.GIMP/config/GIMP/3.2/plug-ins}"
SOURCE="$ROOT/output/gimp-plug-ins/nanotank-systemtest/nanotank-systemtest.py"
INSTALL="$PLUGINS/nanotank-systemtest"
ENTRY="$INSTALL/nanotank-systemtest.py"

printf 'Projekt: %s\n' "$ROOT"
printf 'Erzeugt: %s\n' "$SOURCE"
[[ -f "$SOURCE" && -x "$SOURCE" ]] && echo 'OK: Build ausführbar' || echo 'FEHLT: Build nicht ausführbar'
if [[ -L "$INSTALL" ]]; then
  printf 'Installation: Symlink -> %s\n' "$(readlink -- "$INSTALL")"
elif [[ -f "$INSTALL/.nanotank-managed-origin" ]]; then
  printf 'Installation: verwaltete Kopie von %s\n' "$(cat -- "$INSTALL/.nanotank-managed-origin")"
else
  printf 'Installation: nicht vorhanden oder nicht NanoTank-verwaltet\n'
fi
[[ -f "$ENTRY" && -x "$ENTRY" ]] && echo 'OK: Host kann Plug-in ausführen' || echo 'FEHLT: Host kann Plug-in nicht ausführen'

if command -v flatpak >/dev/null 2>&1; then
  printf '\nPrüfung in der GIMP-Flatpak-Sandbox:\n'
  flatpak run --command=sh org.gimp.GIMP -c '
    echo "XDG_CONFIG_HOME=$XDG_CONFIG_HOME"
    for p in "$1" "$2"; do
      if [ -f "$p" ] && [ -x "$p" ]; then
        printf "OK: %s\n" "$p"
      else
        printf "NICHT ERREICHBAR: %s\n" "$p"
      fi
    done
    if [ -L "$3" ]; then
      printf "GIMP-Plug-in-Verzeichnis ist Symlink: "
      readlink "$3"
    fi
  ' sh "$SOURCE" "$ENTRY" "$INSTALL"
fi
printf '\nIn GIMP ggf. Bearbeiten → Einstellungen → Ordner → Plug-ins kontrollieren.\n'
