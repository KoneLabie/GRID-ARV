#!/usr/bin/env bash
# Collects the build output into dist/stage for zipping.
set -e
PLAT="$1"
P="build/GridARVPlugin_artefacts/Release"
S="build/GridARVStandalone_artefacts/Release"
OUT="dist/stage"
rm -rf dist
mkdir -p "$OUT/Standalone"

[ -d "$P/VST3" ] && cp -R "$P/VST3" "$OUT/"
[ -d "$P/AU" ]   && cp -R "$P/AU"   "$OUT/"
cp -R "$S/"* "$OUT/Standalone/"

case "$PLAT" in
  Windows) cp scripts/install_windows.bat "$OUT/" ;;
  macOS)   cp scripts/install_mac.sh      "$OUT/" ;;
  Linux)   cp scripts/install_linux.sh    "$OUT/" ;;
esac
cp README.md "$OUT/"
echo "Staged:"; find "$OUT" -maxdepth 2
