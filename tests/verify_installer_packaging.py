#!/usr/bin/env python3
"""Dependency-free checks for selectable, direct-download installer packaging."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
checks = {
    "Windows Inno Setup defines selectable VST3, VST2, and standalone components conditionally": (
        ROOT / "installer/windows/GridARV.iss",
        [r"\[Components\]", r'Name: "vst3"', r'Name: "standalone"', r"\{commoncf\}\\VST3", r"#if HasVST2", r'Name: "vst2"', r"\{commoncf\}\\VST2", r"OutputBaseFilename=GridARV-Installer-Windows-\{#InstallerFlavor\}", r"VST2-DISTRIBUTION-NOTICE\.txt"],
    ),
    "Windows build script discovers the built standalone executable": (
        ROOT / "scripts/build_windows_installer.ps1",
        [r"ISCC\.exe", r"Get-ChildItem", r"GridARV\.iss", r"/DHasVST2", r"/DInstallerFlavor", r"Join-Path \$stage 'VST'"],
    ),
    "macOS package script emits component choices": (
        ROOT / "scripts/build_macos_pkg.sh",
        [r"productbuild --distribution", r"choices-outline", r"start_selected=\\\"true\\\"", r"GridARV-Installer-\$\{FLAVOR\}-macOS\.pkg", r'choice_id="vst2"', r"Library/Audio/Plug-Ins/VST", r"VST2-DISTRIBUTION-NOTICE\.txt"],
    ),
    "Linux self-extractor offers selectable install options": (
        ROOT / "scripts/linux_installer_header.sh",
        [r"--vst3", r"--standalone", r"--vst2", r"HAS_VST2", r"Selection \[standard\]", r"VST2-DISTRIBUTION-NOTICE\.txt", r"\.vst", r"tail -c"],
    ),
    "Linux build script creates a directly runnable installer": (
        ROOT / "scripts/build_linux_run.sh",
        [r"GridARV-Installer-\$\{FLAVOR\}-Linux\.run", r"chmod \+x", r"tar -czf"],
    ),
    "Staging includes the VST2 notice only for a VST2 bundle": (
        ROOT / "scripts/package.sh",
        [r"if \[ -d \"?\$OUT/VST", r"VST2-DISTRIBUTION-NOTICE\.txt"],
    ),
    "Build workflow publishes direct installer assets": (
        ROOT / ".github/workflows/build.yml",
        [r"Build Windows menu installer", r"Build macOS component installer", r"Build Linux self-contained installer", r"dist/GridARV-Installer-\*", r"artifacts/GridARV-Installer-\*", r"confirm_vst2_distribution_rights", r"VST2_DISTRIBUTION_AUTHORIZED", r"gridarv-vst2-sdk", r"inputs\.build_vst2"],
    ),
}
failed = []
if (ROOT / ".github/workflows/build-vst2.yml").exists():
    failed.append("VST2 workflow was not consolidated; remove .github/workflows/build-vst2.yml")
# Never accidentally publish Steinberg's restricted VST2 headers with public source.
restricted_headers = {"aeffect.h", "aeffectx.h"}
found_headers = [p for p in ROOT.rglob("*") if p.is_file() and p.name.lower() in restricted_headers
                 and not any(part in {".git", "build", "dist", "__pycache__"} for part in p.parts)]
if found_headers:
    failed.append("restricted VST2 SDK headers must not be committed or packaged: "
                  + ", ".join(str(p.relative_to(ROOT)) for p in found_headers))
gitignore = (ROOT / ".gitignore").read_text(encoding="utf-8") if (ROOT / ".gitignore").exists() else ""
if "/vst2sdk/" not in gitignore or "/VST2_SDK/" not in gitignore:
    failed.append(".gitignore must exclude common private VST2 SDK folder names")

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
