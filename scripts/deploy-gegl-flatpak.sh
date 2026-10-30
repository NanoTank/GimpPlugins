#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Installiert nur eigene GEGL-Module in die *wirkliche* Flatpak-Datenumgebung.
# Funktionierende Symlinks werden bevorzugt; sonst sichere, markierte Kopie.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
OUT="$ROOT/output/gegl"
command -v flatpak >/dev/null || { echo "FEHLER: flatpak fehlt" >&2; exit 1; }

if flatpak ps --columns=application 2>/dev/null | grep -Fxq 'org.gimp.GIMP'; then
  echo "Bitte GIMP vollständig beenden, bevor geladene GEGL-Module aktualisiert werden." >&2
  exit 1
fi

shopt -s nullglob
files=("$OUT"/nanotank-*.so)
if (( ${#files[@]} == 0 )); then
  echo "Keine GEGL-Module in output/gegl/. Zuerst: bash scripts/build-gegl.sh" >&2
  exit 1
fi

for source in "${files[@]}"; do
  # Nur von uns gebaute, reguläre Dateien.
  [[ -f "$source" && ! -L "$source" ]] || {
    echo "Ungültige Quelldatei: $source" >&2
    exit 1
  }
  flatpak run --command=sh org.gimp.GIMP -c '
    set -eu
    source="$1"
    name="${source##*/}"
    case "$name" in nanotank-*.so) ;; *)
      echo "Unerwarteter Modulname: $name" >&2; exit 1;;
    esac
    dest_dir="${XDG_DATA_HOME:?}/gegl-0.4/plug-ins"
    dest="$dest_dir/$name"
    marker="$dest.nanotank-origin"
    mkdir -p "$dest_dir"

    # KEIN Fremdmodul und KEINE unbekannten Links ueberschreiben.
    if [ -L "$dest" ]; then
      [ "$(readlink "$dest")" = "$source" ] || {
        echo "KONFLIKT: Fremder GEGL-Symlink: $dest" >&2; exit 1;
      }
      if [ -r "$dest" ]; then
        echo "Symlink in Flatpak sichtbar: $dest"
        exit 0
      fi
      rm -- "$dest"  # nur unser eigener Link
    elif [ -e "$dest" ]; then
      [ -f "$dest" ] && [ -f "$marker" ] &&
      [ "$(cat "$marker")" = "$source" ] || {
        echo "KONFLIKT: Unbekannte GEGL-Datei: $dest" >&2; exit 1;
      }
    else
      ln -s -- "$source" "$dest"
      if [ -r "$dest" ]; then
        echo "Symlink in Flatpak sichtbar: $dest"
        exit 0
      fi
      rm -- "$dest"
    fi

    # Staging-Datei zuerst vollstaendig schreiben und atomar ersetzen.
    tmp="$(mktemp "$dest_dir/.nanotank-gegl.XXXXXXXX")"
    trap '\''rm -f -- "$tmp"'\'' EXIT
    cp -- "$source" "$tmp"
    chmod 0644 "$tmp"
    mv -f -- "$tmp" "$dest"
    printf "%s\n" "$source" > "$marker"
    test -r "$dest" || {
      echo "FEHLER: GEGL-Modul ist in der Flatpak-Sandbox nicht lesbar" >&2
      exit 1
    }
    echo "Verwaltete GEGL-Kopie in Flatpak installiert: $dest"
  ' sh "$source"
done
printf '\nGIMP neu starten und Filter → NanoTank → GEGL-Test prüfen.\n'
