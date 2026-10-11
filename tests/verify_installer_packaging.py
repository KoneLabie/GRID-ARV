#!/usr/bin/env python3
"""Dependency-free checks for selectable, direct-download installer packaging."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
checks = {
    "Windows Inno Setup defines selectable VST3 and standalone components": (
        ROOT / "installer/windows/GridARV.iss",
        [r"\[Components\]", r'Name: "vst3"', r'Name: "standalone"', r"\{commoncf\}\\VST3"],
    ),
    "Windows build script discovers the built standalone executable": (
        ROOT / "scripts/build_windows_installer.ps1",
        [r"ISCC\.exe", r"Get-ChildItem", r"GridARV\.iss"],
    ),
    "macOS package script emits component choices": (
        ROOT / "scripts/build_macos_pkg.sh",
        [r"productbuild --distribution", r"choices-outline", r"start_selected=\\\"true\\\"", r"GridARV-Installer-macOS\.pkg"],
    ),
    "Linux self-extractor offers selectable install options": (
        ROOT / "scripts/linux_installer_header.sh",
        [r"--vst3", r"--standalone", r"--all", r"Selection \[3\]", r"tail -c"],
    ),
    "Linux build script creates a directly runnable installer": (
        ROOT / "scripts/build_linux_run.sh",
        [r"GridARV-Installer-Linux\.run", r"chmod \+x", r"tar -czf"],
    ),
    "Build workflow publishes direct installer assets": (
        ROOT / ".github/workflows/build.yml",
        [r"Build Windows menu installer", r"Build macOS component installer", r"Build Linux self-contained installer", r"dist/GridARV-Installer-\*", r"artifacts/GridARV-Installer-\*"],
    ),
}
failed = []
for label, (path, patterns) in checks.items():
    if not path.is_file():
        failed.append(f"{label}: missing {path.relative_to(ROOT)}")
        continue
    content = path.read_text(encoding="utf-8")
    missing = [pattern for pattern in patterns if not re.search(pattern, content, re.MULTILINE)]
    if missing:
        failed.append(f"{label}: patterns not found: {missing}")
    else:
        print(f"PASS: {label}")
if failed:
    for item in failed:
        print(f"FAIL: {item}", file=sys.stderr)
    raise SystemExit(1)
print(f"All {len(checks)} installer packaging checks passed.")
