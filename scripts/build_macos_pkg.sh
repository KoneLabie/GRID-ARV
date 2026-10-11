#!/usr/bin/env bash
# Build a single macOS installer with selectable VST3, AU, and standalone components.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
STAGE="$ROOT/dist/stage"
WORK="$ROOT/dist/macos-installer-work"
COMPONENTS="$WORK/components"
OUT="$ROOT/dist/GridARV-Installer-macOS.pkg"
VERSION="$(sed -n 's/^project(GridARV VERSION \([^ ]*\).*/\1/p' "$ROOT/CMakeLists.txt" | head -n1)"
VERSION="${VERSION:-1.0.0}"

if [[ ! -d "$STAGE" ]]; then echo "Missing stage folder: $STAGE" >&2; exit 1; fi
rm -rf "$WORK"
mkdir -p "$COMPONENTS"

CHOICES=()
PKG_REFS=()

build_component() {
  local choice_id="$1" title="$2" description="$3" identifier="$4" payload_path="$5" source_path="$6"
  local payload="$WORK/payload-$choice_id"
  mkdir -p "$payload/$payload_path"
  cp -R "$source_path"/. "$payload/$payload_path/"
  pkgbuild --root "$payload" --identifier "$identifier" --version "$VERSION" \
    --install-location / "$COMPONENTS/$choice_id.pkg"
  CHOICES+=("<line choice=\"$choice_id\"/>")
  PKG_REFS+=("<choice id=\"$choice_id\" title=\"$title\" description=\"$description\" start_selected=\"true\"><pkg-ref id=\"$identifier\"/></choice>" \
             "<pkg-ref id=\"$identifier\" version=\"$VERSION\" onConclusion=\"none\">$choice_id.pkg</pkg-ref>")
}

if [[ -d "$STAGE/VST3" ]] && find "$STAGE/VST3" -mindepth 1 -maxdepth 1 -print -quit | grep -q .; then
  build_component "vst3" "VST3 Plug-in" "Install Grid ARV for VST3-compatible DAWs." \
    "com.konelabie.gridarv.vst3" "Library/Audio/Plug-Ins/VST3" "$STAGE/VST3"
fi
if [[ -d "$STAGE/AU" ]] && find "$STAGE/AU" -mindepth 1 -maxdepth 1 -print -quit | grep -q .; then
  build_component "au" "Audio Units Plug-in" "Install Grid ARV for Audio Units-compatible DAWs." \
    "com.konelabie.gridarv.au" "Library/Audio/Plug-Ins/Components" "$STAGE/AU"
fi
APP_PATH=""
if [[ -d "$STAGE/Standalone" ]]; then
  while IFS= read -r -d '' candidate; do APP_PATH="$candidate"; break; done < <(find "$STAGE/Standalone" -maxdepth 2 -type d -name '*.app' -print0)
fi
if [[ -n "$APP_PATH" ]]; then
  # Preserve the .app directory itself inside /Applications instead of copying
  # its contents directly into the Applications folder.
  choice_id="standalone"
  identifier="com.konelabie.gridarv.standalone"
  payload="$WORK/payload-$choice_id"
  app_dest="$payload/Applications/$(basename "$APP_PATH")"
  mkdir -p "$app_dest"
  cp -R "$APP_PATH"/. "$app_dest/"
  pkgbuild --root "$payload" --identifier "$identifier" --version "$VERSION" \
    --install-location / "$COMPONENTS/$choice_id.pkg"
  CHOICES+=("<line choice=\"$choice_id\"/>")
  PKG_REFS+=("<choice id=\"$choice_id\" title=\"Standalone App\" description=\"Install the standalone Grid ARV application in /Applications.\" start_selected=\"true\"><pkg-ref id=\"$identifier\"/></choice>" \
             "<pkg-ref id=\"$identifier\" version=\"$VERSION\" onConclusion=\"none\">$choice_id.pkg</pkg-ref>")
else
  echo "No macOS .app bundle found in $STAGE/Standalone; standalone component will be omitted." >&2
fi

if [[ ${#CHOICES[@]} -eq 0 ]]; then echo "No installable macOS components were staged." >&2; exit 1; fi
{
  cat <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>Grid ARV Installer</title>
  <options customize="always" require-scripts="false" hostArchitectures="arm64,x86_64"/>
  <choices-outline>
XML
  for line in "${CHOICES[@]}"; do echo "    $line"; done
  echo '  </choices-outline>'
  for item in "${PKG_REFS[@]}"; do echo "  $item"; done
  echo '</installer-gui-script>'
} > "$WORK/distribution.xml"

productbuild --distribution "$WORK/distribution.xml" --package-path "$COMPONENTS" "$OUT"
echo "Created $OUT"
