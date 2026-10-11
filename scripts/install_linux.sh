#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
if [ -d VST3 ]; then mkdir -p "$HOME/.vst3"; cp -R VST3/*.vst3 "$HOME/.vst3/" 2>/dev/null || true; fi
chmod +x Standalone/* 2>/dev/null || true
echo "VST3 installed if present. Standalone files remain in the Standalone folder."
