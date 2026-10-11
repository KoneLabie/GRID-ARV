#!/usr/bin/env python3
"""End-to-end test the Linux self-extracting installer without a native GridARV build."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def run(command, *, env=None, cwd=None):
    result = subprocess.run(command, cwd=cwd, env=env, text=True, capture_output=True)
    if result.returncode != 0:
        raise RuntimeError(
            f"Command failed ({result.returncode}): {' '.join(map(str, command))}\n"
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
    installer = repo / "dist" / "GridARV-Installer-Linux.run"
    if not installer.is_file() or not os.access(installer, os.X_OK):
        raise RuntimeError("Linux installer was not created as an executable file")

    home_vst3 = temp / "home-vst3"
    env_vst3 = os.environ.copy()
    env_vst3["HOME"] = str(home_vst3)
    run(["bash", str(installer), "--vst3"], env=env_vst3)
    if not (home_vst3 / ".vst3/Test.vst3/Contents/manifest.txt").is_file():
        raise RuntimeError("--vst3 did not install the VST3 bundle")

    home_app = temp / "home-app"
    env_app = os.environ.copy()
    env_app["HOME"] = str(home_app)
    run(["bash", str(installer), "--standalone"], env=env_app)
    if not os.access(home_app / ".local/opt/GridARV/Standalone/GridARV-Test-App", os.X_OK):
        raise RuntimeError("--standalone did not install the application executable")
    if not (home_app / ".local/share/applications/gridarv.desktop").is_file():
        raise RuntimeError("--standalone did not create a desktop-menu entry")

print("PASS: Linux self-extracting installer packages successfully")
print("PASS: --vst3 installs the VST3 bundle to the user-local plug-in directory")
print("PASS: --standalone installs the app and desktop-menu entry")
