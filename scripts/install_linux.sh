#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
if [ -d VST3 ]; then mkdir -p "$HOME/.vst3"; cp -R VST3/*.vst3 "$HOME/.vst3/" 2>/dev/null || true; fi
chmod +x Standalone/* 2>/dev/null || true
if [ -d VST ]; then
  echo "A VST2 plug-in is present in this package. VST2 should only be distributed with valid pre-existing written Steinberg permission."
  if [[ -t 0 ]]; then
    read -r -p "Install the VST2 plug-in to ~/.vst? [y/N] " answer
    case "$answer" in
      y|Y|yes|YES)
        mkdir -p "$HOME/.vst"
        cp -R VST/* "$HOME/.vst/"
        if [ -f VST2-DISTRIBUTION-NOTICE.txt ]; then
          mkdir -p "$HOME/.local/share/doc/GridARV"
          cp VST2-DISTRIBUTION-NOTICE.txt "$HOME/.local/share/doc/GridARV/"
        fi
        echo "Installed VST2 plug-in to ~/.vst"
        ;;
      *) echo "Skipped VST2 installation." ;;
    esac
  else
    echo "Skipped VST2 installation because no interactive terminal was available. Use the selectable GridARV .run installer to choose components."
  fi
fi
