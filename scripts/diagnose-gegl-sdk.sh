#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Nur lesende Diagnose in der GIMP-Flatpak-SDK-Sandbox.
set -euo pipefail
command -v flatpak >/dev/null 2>&1 || { echo "FEHLER: flatpak fehlt" >&2; exit 1; }

flatpak run --devel --command=sh org.gimp.GIMP -c '
  printf "Flatpak-GIMP-SDK Diagnose\n"
  printf "PKG_CONFIG_PATH=%s\n" "${PKG_CONFIG_PATH-<nicht gesetzt>}"
  printf "PKG_CONFIG_LIBDIR=%s\n" "${PKG_CONFIG_LIBDIR-<nicht gesetzt>}"
  printf "Compiler: "; command -v cc || true
  printf "pkg-config: "; command -v pkg-config || true
  echo "GEGL pkg-config-Dateien unter /app:"
  find /app -name "gegl-0.4.pc" -print 2>/dev/null | head -20
  echo "GEGL-Operationsheader unter /app:"
  find /app -name "gegl-op.h" -print 2>/dev/null | head -20
  echo "GEGL-Laufzeitbibliotheken unter /app:"
  find /app/lib /app/lib64 -maxdepth 3 -name "libgegl-0.4.so*" -print 2>/dev/null | head -20
  echo "pkg-config --modversion gegl-0.4 (Standardumgebung):"
  pkg-config --modversion gegl-0.4 2>&1 || true
  if [ -f /app/lib/pkgconfig/gegl-0.4.pc ]; then
    echo "Probe mit PKG_CONFIG_PATH=/app/lib/pkgconfig:"
    PKG_CONFIG_PATH="/app/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}" pkg-config --modversion gegl-0.4 2>&1 || true
  fi
'
