#!/usr/bin/env bash
cd "$(dirname "$0")"
mkdir -p ~/Library/Audio/Plug-Ins/VST3 ~/Library/Audio/Plug-Ins/Components
cp -R VST3/*.vst3 ~/Library/Audio/Plug-Ins/VST3/ 2>/dev/null
cp -R AU/*.component ~/Library/Audio/Plug-Ins/Components/ 2>/dev/null
# builds are unsigned: remove the download quarantine flag
xattr -cr ~/Library/Audio/Plug-Ins/VST3/*Grid*ARV* ~/Library/Audio/Plug-Ins/Components/*Grid*ARV* Standalone 2>/dev/null
echo "Installed. Rescan plugins in your DAW. Standalone app: ./Standalone"
