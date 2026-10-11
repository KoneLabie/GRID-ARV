#!/usr/bin/env bash
# Bundle staged files into one directly runnable Linux installer.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
STAGE="$ROOT/dist/stage"
OUT="$ROOT/dist/GRID-ARV_Installer-Linux.run"
PAYLOAD="$ROOT/dist/gridarv-payload.tar.gz"
HEADER="$ROOT/scripts/linux_installer_header.sh"

if [[ ! -d "$STAGE" ]]; then echo "Missing stage folder: $STAGE" >&2; exit 1; fi
mkdir -p "$ROOT/dist"
tar -czf "$PAYLOAD" -C "$STAGE" .
bytes="$(wc -c < "$PAYLOAD" | tr -d '[:space:]')"
sed "s/@PAYLOAD_BYTES@/$bytes/" "$HEADER" > "$OUT"
cat "$PAYLOAD" >> "$OUT"
chmod +x "$OUT"
rm -f "$PAYLOAD"
echo "Created $OUT"
