#!/usr/bin/env python3
"""Dependency-free regression checks for the compact VST3/AU/standalone installers."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
checks = {
    "Windows installer offers VST3 and standalone": (
        ROOT / "installer/windows/GridARV.iss",
        [r"\[Components\]", r'Name: "vst3"', r'Name: "standalone"',
         r"\{commoncf\}\\VST3", r"OutputBaseFilename=GridARV-Installer-Windows-Standard"],
    ),
    "Windows installer script finds the standalone app": (
        ROOT / "scripts/build_windows_installer.ps1",
        [r"ISCC\.exe", r"Get-ChildItem", r"GridARV\.iss", r"/DStandaloneExe="],
    ),
    "macOS installer offers VST3 AU and standalone": (
        ROOT / "scripts/build_macos_pkg.sh",
        [r"productbuild --distribution", r"choices-outline", r"start_selected=\\\"true\\\"",
         r"GridARV-Installer-Standard-macOS\.pkg", r'build_component "vst3"', r'build_component "au"',
         r'choice_id="standalone"'],
    ),
    "Linux installer offers VST3 and standalone": (
        ROOT / "scripts/linux_installer_header.sh",
        [r"--vst3", r"--standalone", r"--all", r"Selection \[standard\]", r"\.vst3", r"tail -c"],
    ),
    "Linux self-extractor builds direct installer": (
        ROOT / "scripts/build_linux_run.sh",
        [r"GridARV-Installer-Standard-Linux\.run", r"chmod \+x", r"tar -czf"],
    ),
    "Build workflow publishes direct installer assets": (
        ROOT / ".github/workflows/build.yml",
        [r"Build Windows menu installer", r"Build macOS component installer",
         r"Build Linux self-contained installer", r"dist/GridARV-Installer-\*",
         r"artifacts/GridARV-Installer-\*", r"ubuntu-22\.04", r"windows-latest", r"macos-latest"],
    ),
    "CMake selects standard formats": (
        ROOT / "CMakeLists.txt",
        [r"set\(PLUGIN_FORMATS VST3\)", r"list\(APPEND PLUGIN_FORMATS AU\)",
         r"JUCE_TAG \"8\.0\.4\""],
    ),
}
failed = []
for label, (path, patterns) in checks.items():
    if not path.is_file():
        failed.append(f"{label}: missing {path.relative_to(ROOT)}")
        continue
    content = path.read_text(encoding="utf-8")
    missing = [pat for pat in patterns if not re.search(pat, content, re.MULTILINE)]
    if missing:
        failed.append(f"{label}: missing {missing}")
    else:
        print(f"PASS: {label}")

# Windows PowerShell can split an unquoted dotted -D variable into `8` and `.0.4`.
# Keep JUCE version pinned in CMake rather than passing it as a workflow CLI argument.
workflow_text = (ROOT / ".github/workflows/build.yml").read_text(encoding="utf-8")
cmake_text = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
if re.search(r"-DJUCE_TAG\s*=", workflow_text, re.I):
    failed.append("workflow passes JUCE version via a shell -D argument")
if not re.search(r"GIT_TAG\s+\$\{JUCE_TAG\}", cmake_text):
    failed.append("JUCE FetchContent does not use the pinned CMake tag")
if not re.search(r"run:\s*cmake -S \. -B build -DCMAKE_BUILD_TYPE=Release", workflow_text):
    failed.append("workflow no longer configures with a shell-safe CMake command")

# Prohibit an old workflow, SDK files, VST2 build toggles, and installer integration.
if (ROOT / ".github/workflows/build-vst2.yml").exists():
    failed.append("obsolete VST2 workflow still exists")
for path in ROOT.rglob("*"):
    if not path.is_file() or any(x in {".git", "build", "dist", "__pycache__"} for x in path.parts):
        continue
    if path.name.lower() in {"aeffect.h", "aeffectx.h"}:
        failed.append(f"obsolete restricted header present: {path.relative_to(ROOT)}")
    if path.suffix not in {".yml", ".sh", ".ps1", ".iss", ".txt", ".md"} and path.name != "CMakeLists.txt":
        continue
    if path == ROOT / "README.md" or path == ROOT / ".gitignore":
        continue
    if re.search(r"VST2|VST2_SDK|BUILD_VST2|ENABLE_VST2|confirm_vst2|--vst2|[\\/]VST[\\/]", path.read_text(encoding="utf-8"), re.I):
        failed.append(f"unexpected legacy VST2 integration: {path.relative_to(ROOT)}")
if failed:
    for message in failed:
        print(f"FAIL: {message}", file=sys.stderr)
    raise SystemExit(1)
print(f"PASS: all {len(checks)} packaging checks and VST2 removal checks passed")
