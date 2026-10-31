#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Installiert nur eigene GEGL-Module in die *wirkliche* Flatpak-Datenumgebung.
# Funktionierende Symlinks werden bevorzugt; sonst sichere, markierte Kopie.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
OUT="$ROOT/output/gegl"
command -v flatpak >/dev/null || { echo "FEHLER: flatpak fehlt" >&2; exit 1; }

# Der unmittelbar vorherige Build startet selbst eine GIMP-Flatpak-Sandbox
# (--devel --command=sh). Deren Flatpak-Instanz kann nach dem beendeten Build
# noch kurz in "flatpak ps" sichtbar sein. Nicht sofort fälschlich abbrechen,
# aber niemals ein geladenes GEGL-Modul in einer laufenden GIMP-Sitzung ersetzen.
gimp_instance_running() {
  flatpak ps --columns=application 2>/dev/null | grep -Fxq 'org.gimp.GIMP'
}

if gimp_instance_running; then
  echo "GIMP-Flatpak-Instanz erkannt; prüfe kurz auf auslaufende Build-Sandbox ..."
  for attempt in 1 2 3 4 5 6 7 8 9 10; do
    sleep 0.3
    if ! gimp_instance_running; then
      break
    fi
  done
fi

if gimp_instance_running; then
  echo "GIMP-Flatpak ist nach der kurzen Wartezeit weiterhin registriert." >&2
  echo "Zum Schutz vor Austausch geladener GEGL-Module wird nicht installiert." >&2
  echo "Zur Diagnose laufender Instanzen:" >&2
  flatpak ps --columns=instance,application,pid,child-pid >&2 || true
  exit 1
fi

# Optional genau ein Modul installieren, ohne andere NanoTank-Module
# erneut anzufassen: bash scripts/deploy-gegl-flatpak.sh nanotank-glass
shopt -s nullglob
if (( $# > 0 )); then
  case "$1" in
    nanotank-gegl-test|nanotank-glass|nanotank-glass-refraction) ;;
    *) echo "Unbekanntes NanoTank-Modul: $1" >&2; exit 2 ;;
  esac
  files=("$OUT/$1.so")
else
  files=("$OUT"/nanotank-*.so)
fi
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
printf '\nGIMP vollständig neu starten und NanoTank-Menü prüfen.\n'
