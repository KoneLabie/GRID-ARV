#!/usr/bin/env bash
# Grid ARV self-extracting Linux installer. The compressed payload is appended below.
set -euo pipefail
PAYLOAD_BYTES='@PAYLOAD_BYTES@'

show_help() {
  cat <<HELP
Grid ARV installer

Run this file in a terminal to choose what to install.
  1  VST3 plug-in (user-local: ~/.vst3)
  2  Standalone app (user-local: ~/.local/opt/GridARV)
  3  All available components

Options:
  --help       Show this help
  --vst3       Install only the VST3 plug-in (non-interactive)
  --standalone Install only the standalone app (non-interactive)
  --all        Install all available components (non-interactive)
HELP
}

case "${1:-}" in
  --help|-h) show_help; exit 0 ;;
esac
TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/gridarv-installer.XXXXXX")"
trap 'rm -rf "$TMP_DIR"' EXIT
if ! tail -c "$PAYLOAD_BYTES" "$0" | tar -xz -C "$TMP_DIR"; then
  echo 'Could not unpack the installer payload.' >&2
  exit 1
fi

HAS_VST3=0
HAS_STANDALONE=0
[[ -d "$TMP_DIR/VST3" ]] && HAS_VST3=1
[[ -d "$TMP_DIR/Standalone" ]] && HAS_STANDALONE=1
if [[ $HAS_VST3 -eq 0 && $HAS_STANDALONE -eq 0 ]]; then
  echo 'This installer contains no recognized components.' >&2
  exit 1
fi

INSTALL_VST3=0
INSTALL_STANDALONE=0
if [[ $# -gt 0 ]]; then
  for arg in "$@"; do
    case "$arg" in
      --vst3) INSTALL_VST3=1 ;;
      --standalone) INSTALL_STANDALONE=1 ;;
      --all) INSTALL_VST3=$HAS_VST3; INSTALL_STANDALONE=$HAS_STANDALONE ;;
      *) echo "Unknown option: $arg" >&2; show_help; exit 2 ;;
    esac
  done
else
  if [[ ! -t 0 ]]; then
    echo 'Run this installer in a terminal so you can choose components.' >&2
    show_help
    exit 2
  fi
  echo 'Grid ARV Installer'
  echo 'Choose what to install (enter one or more numbers separated by spaces):'
  [[ $HAS_VST3 -eq 1 ]] && echo '  1) VST3 plug-in'
  [[ $HAS_STANDALONE -eq 1 ]] && echo '  2) Standalone app'
  echo '  3) All available components'
  read -r -p 'Selection [standard]: ' selection
  selection="${selection:-standard}"
  if [[ "$selection" == "standard" || "$selection" == "3" || "$selection" == "all" || "$selection" == "both" ]]; then
    INSTALL_VST3=$HAS_VST3
    INSTALL_STANDALONE=$HAS_STANDALONE
  else
    [[ " $selection " == *' 1 '* && $HAS_VST3 -eq 1 ]] && INSTALL_VST3=1
    [[ " $selection " == *' 2 '* && $HAS_STANDALONE -eq 1 ]] && INSTALL_STANDALONE=1
    if [[ $INSTALL_VST3 -eq 0 && $INSTALL_STANDALONE -eq 0 ]]; then
      echo 'Nothing selected or selected component is not included; no changes made.'
      exit 0
    fi
  fi
fi

if [[ $INSTALL_VST3 -eq 1 && $HAS_VST3 -eq 0 ]]; then echo 'VST3 component is not present in this package.' >&2; exit 2; fi
if [[ $INSTALL_STANDALONE -eq 1 && $HAS_STANDALONE -eq 0 ]]; then echo 'Standalone component is not present in this package.' >&2; exit 2; fi

if [[ $INSTALL_VST3 -eq 1 ]]; then
  mkdir -p "$HOME/.vst3"
  shopt -s nullglob
  for bundle in "$TMP_DIR"/VST3/*; do
    [[ -e "$bundle" ]] || continue
    destination="$HOME/.vst3/$(basename "$bundle")"
    rm -rf "$destination"
    cp -R "$bundle" "$HOME/.vst3/"
  done
  echo 'Installed VST3 plug-in to ~/.vst3'
fi

if [[ $INSTALL_STANDALONE -eq 1 ]]; then
  APP_DIR="$HOME/.local/opt/GridARV"
  mkdir -p "$HOME/.local/opt" "$HOME/.local/share/applications"
  rm -rf "$APP_DIR/Standalone"
  mkdir -p "$APP_DIR"
  cp -R "$TMP_DIR/Standalone" "$APP_DIR/Standalone"
  APP_EXEC="$(find "$APP_DIR/Standalone" -type f -perm /111 -print -quit || true)"
  if [[ -z "$APP_EXEC" ]]; then
    echo 'Standalone files were copied, but no executable was found. Check the package build.' >&2
    exit 1
  fi
  cat > "$HOME/.local/share/applications/gridarv.desktop" <<DESKTOP
[Desktop Entry]
Type=Application
Name=Grid ARV
Comment=Audio Reactive Visualizer
Exec="$APP_EXEC"
Path="$APP_DIR/Standalone"
Terminal=false
Categories=AudioVideo;Audio;Video;
StartupNotify=true
DESKTOP
  chmod 644 "$HOME/.local/share/applications/gridarv.desktop"
  echo 'Installed standalone app to ~/.local/opt/GridARV and added a desktop-menu entry.'
fi

echo 'Grid ARV installation complete. Rescan plug-ins in your DAW if you installed VST3.'
exit 0
__GRIDARV_PAYLOAD_BELOW__
