#!/usr/bin/env bash
cd "$(dirname "$0")"
mkdir -p ~/.vst3
cp -R VST3/*.vst3 ~/.vst3/
chmod +x Standalone/* 2>/dev/null
echo "Installed to ~/.vst3. Standalone app: ./Standalone"
