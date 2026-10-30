#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Symlink when visible in the sandbox, otherwise a managed copy.
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
GIMP_PLUGINS="${NANOTANK_GIMP_PLUGINS_DIR:-$HOME/.var/app/org.gimp.GIMP/config/GIMP/3.2/plug-ins}"
GEGL_PLUGINS="${NANOTANK_GEGL_PLUGINS_DIR:-$HOME/.var/app/org.gimp.GIMP/data/gegl-0.4/plug-ins}"
OUT="$ROOT/output"
if [[ ! -d "$OUT/gimp-plug-ins" ]]; then
  printf 'Zuerst: bash scripts/build.sh\n' >&2
  exit 1
fi
mkdir -p -- "$GIMP_PLUGINS" "$GEGL_PLUGINS"

# Test the destination as GIMP sees it, not the host's symlink resolution.
sandbox_can_read() {
  local target="$1" executable="$2"
  command -v flatpak >/dev/null 2>&1 || return 1
  flatpak run --command=sh org.gimp.GIMP -c '
    test -f "$1" && test -r "$1" && { test "$2" != yes || test -x "$1"; }
  ' sh "$target" "$executable" >/dev/null 2>&1
}

# Build a private staging copy and replace only a previously-owned installation.
# The marker records the original source and is checked before updating/removing.
copy_plugin() {
  local source="$1" dest="$2" entry="$3" stage old="" parent
  parent="$(dirname -- "$dest")"
  stage="$(mktemp -d "$parent/.nanotank-stage.XXXXXXXX")"
  if ! cp -a -- "$source/." "$stage/"; then
    rm -rf -- "$stage"
    return 1
  fi
  printf '%s\n' "$source" > "$stage/.nanotank-managed-origin"
  if [[ -L "$dest" ]]; then
    rm -- "$dest"
  elif [[ -d "$dest" ]]; then
    old="$(mktemp -d "$parent/.nanotank-old.XXXXXXXX")"
    rmdir -- "$old"
    mv -- "$dest" "$old"
  fi
  if ! mv -- "$stage" "$dest"; then
    [[ -z "$old" ]] || mv -- "$old" "$dest"
    return 1
  fi
  [[ -z "$old" ]] || rm -rf -- "$old"
  if sandbox_can_read "$dest/$entry" yes; then
    printf 'Kopie installiert und in GIMP-Flatpak erreichbar: %s\n' "$dest"
  else
    printf 'FEHLER: Auch die Kopie ist in GIMP-Flatpak nicht erreichbar: %s\n' "$dest" >&2
    printf 'Bitte in GIMP als zusätzliches Plug-in-Verzeichnis eintragen: %s\n' "$ROOT/output/gimp-plug-ins" >&2
    return 1
  fi
}

install_plugin() {
  local source="$1" dest="$2" name entry
  name="$(basename -- "$source")"
  entry="$name.py"
  [[ -x "$source/$entry" ]] || entry="$name"
  if [[ ! -f "$source/$entry" || ! -x "$source/$entry" ]]; then
    printf 'Fehlender ausführbarer Einstiegspunkt: %s\n' "$source" >&2
    return 1
  fi

  if [[ -L "$dest" ]]; then
    if [[ "$(readlink -- "$dest")" != "$source" ]]; then
      printf 'KONFLIKT (fremder Symlink): %s\n' "$dest" >&2
      return 1
    fi
  elif [[ -e "$dest" ]]; then
    if [[ ! -d "$dest" || ! -f "$dest/.nanotank-managed-origin" ||
          "$(cat -- "$dest/.nanotank-managed-origin")" != "$source" ]]; then
      printf 'KONFLIKT (fremde Installation): %s\n' "$dest" >&2
      return 1
    fi
    copy_plugin "$source" "$dest" "$entry"
    return
  else
    ln -s -- "$source" "$dest"
  fi

  if sandbox_can_read "$dest/$entry" yes; then
    printf 'Sandbox-Symlink funktioniert: %s\n' "$dest"
  else
    printf 'Symlink in Flatpak nicht erreichbar, verwende Kopie: %s\n' "$dest"
    copy_plugin "$source" "$dest" "$entry"
  fi
}

# .so files use a small sidecar ownership marker only for managed copies.
install_gegl() {
  local source="$1" dest="$2" marker stage
  marker="$(dirname -- "$dest")/.nanotank-$(basename -- "$source").origin"
  if [[ -L "$dest" ]]; then
    if [[ "$(readlink -- "$dest")" != "$source" ]]; then
      printf 'KONFLIKT (fremder GEGL-Symlink): %s\n' "$dest" >&2
      return 1
    fi
  elif [[ -e "$dest" ]]; then
    if [[ ! -f "$marker" || "$(cat -- "$marker")" != "$source" ]]; then
      printf 'KONFLIKT (fremder GEGL-Filter): %s\n' "$dest" >&2
      return 1
    fi
    stage="$(mktemp "$(dirname -- "$dest")/.nanotank-gegl.XXXXXXXX")"
    cp -p -- "$source" "$stage"
    mv -f -- "$stage" "$dest"
    if sandbox_can_read "$dest" no; then
      printf 'GEGL-Kopie in Flatpak erreichbar: %s\n' "$dest"
    else
      printf 'FEHLER: GEGL-Kopie im Flatpak nicht erreichbar: %s\n' "$dest" >&2
      return 1
    fi
    return
  else
    ln -s -- "$source" "$dest"
  fi

  if sandbox_can_read "$dest" no; then
    printf 'GEGL-Symlink funktioniert: %s\n' "$dest"
  else
    printf 'GEGL-Symlink in Flatpak nicht erreichbar, verwende Kopie: %s\n' "$dest"
    stage="$(mktemp "$(dirname -- "$dest")/.nanotank-gegl.XXXXXXXX")"
    cp -p -- "$source" "$stage"
    rm -- "$dest"    # Only our own symlink, verified above.
    mv -- "$stage" "$dest"
    printf '%s\n' "$source" > "$marker"
    if ! sandbox_can_read "$dest" no; then
      printf 'FEHLER: GEGL-Kopie im Flatpak nicht erreichbar: %s\n' "$dest" >&2
      return 1
    fi
  fi
}

shopt -s nullglob
for source in "$OUT/gimp-plug-ins"/nanotank-*; do
  [[ -d "$source" ]] || continue
  install_plugin "$source" "$GIMP_PLUGINS/$(basename -- "$source")"
done
for source in "$OUT/gegl"/nanotank-*.so; do
  [[ -f "$source" ]] || continue
  install_gegl "$source" "$GEGL_PLUGINS/$(basename -- "$source")"
done
printf '\nInstallation fertig. GIMP komplett neu starten.\n'
printf 'Bei direkter GIMP-Suchpfad-Konfiguration reicht bash scripts/build.sh.\n'
printf 'Bei Verwendung der alternativen Kopierinstallation: bash scripts/install-flatpak.sh\n'
