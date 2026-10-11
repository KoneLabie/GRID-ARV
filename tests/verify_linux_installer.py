#!/usr/bin/env python3
"""End-to-end Linux installer test with fake standard-component payloads."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def run(command, *, env=None, cwd=None, success=True):
    result = subprocess.run(command, cwd=cwd, env=env, text=True, capture_output=True)
    if (result.returncode == 0) != success:
        raise RuntimeError(
            f"Unexpected return code {result.returncode}: {' '.join(map(str, command))}\n"
            f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
        )
    return result

with tempfile.TemporaryDirectory(prefix="gridarv-installer-test-") as temp_dir:
    temp = Path(temp_dir)
    repo = temp / "repo"
    scripts = repo / "scripts"
    stage = repo / "dist" / "stage"
    (stage / "VST3" / "Test.vst3" / "Contents").mkdir(parents=True)
    (stage / "Standalone").mkdir(parents=True)
    scripts.mkdir(parents=True)
    shutil.copy2(ROOT / "scripts/build_linux_run.sh", scripts)
    shutil.copy2(ROOT / "scripts/linux_installer_header.sh", scripts)
    (repo / "CMakeLists.txt").write_text("project(GridARV VERSION 1.0.0 LANGUAGES C CXX)\n")
    (stage / "VST3" / "Test.vst3" / "Contents" / "manifest.txt").write_text("sample bundle\n")
    app = stage / "Standalone" / "GridARV-Test-App"
    app.write_text("#!/usr/bin/env sh\necho test\n")
    app.chmod(0o755)

    run(["bash", str(scripts / "build_linux_run.sh")], cwd=repo)
    installer = repo / "dist" / "GridARV-Installer-Standard-Linux.run"
    if not installer.is_file() or not os.access(installer, os.X_OK):
        raise RuntimeError("Linux installer was not created as an executable file")

    for arg in ("--vst3", "--standalone", "--all"):
        home = temp / f"home-{arg[2:]}"
        env = os.environ.copy()
        env["HOME"] = str(home)
        run(["bash", str(installer), arg], env=env)
        vst3 = home / ".vst3/Test.vst3/Contents/manifest.txt"
        exe = home / ".local/opt/GridARV/Standalone/GridARV-Test-App"
        desktop = home / ".local/share/applications/gridarv.desktop"
        if (arg in ("--vst3", "--all")) != vst3.is_file():
            raise RuntimeError(f"VST3 install selection incorrect for {arg}")
        if (arg in ("--standalone", "--all")) != (os.access(exe, os.X_OK) and desktop.is_file()):
            raise RuntimeError(f"Standalone install selection incorrect for {arg}")

    home_legacy = temp / "home-legacy"
    env_legacy = os.environ.copy()
    env_legacy["HOME"] = str(home_legacy)
    rejected = run(["bash", str(installer), "--vst2"], env=env_legacy, success=False)
    if "Unknown option" not in rejected.stderr:
        raise RuntimeError("Obsolete VST2 installer option was not rejected cleanly")

print("PASS: Linux installer generates as executable single-file .run")
print("PASS: --vst3, --standalone, and --all choose only the requested components")
print("PASS: legacy --vst2 option is unavailable")
