$ErrorActionPreference = 'Stop'
$stage = Join-Path (Get-Location) 'dist\stage'
$iscc = Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe'
if (-not (Test-Path $stage)) { throw "Staging folder not found: $stage" }
if (-not (Test-Path $iscc)) { throw "Inno Setup compiler not found: $iscc" }

$standalone = Get-ChildItem (Join-Path $stage 'Standalone') -Filter '*.exe' -File -Recurse | Select-Object -First 1
if (-not $standalone) { throw 'Could not find the standalone Grid ARV executable in dist/stage/Standalone.' }
$relativeExe = [System.IO.Path]::GetRelativePath((Join-Path $stage 'Standalone'), $standalone.FullName) -replace '/', '\'
$hasVst2 = 0
$vst2Dir = Join-Path $stage 'VST'
if ((Test-Path $vst2Dir) -and (Get-ChildItem $vst2Dir -Force | Select-Object -First 1)) { $hasVst2 = 1 }
$flavor = if ($env:BUILD_VST2 -eq 'true') { 'VST2' } else { 'Standard' }
$versionLine = Select-String -Path 'CMakeLists.txt' -Pattern '^project\(GridARV VERSION ([^ ]+)' | Select-Object -First 1
$version = if ($versionLine) { $versionLine.Matches[0].Groups[1].Value } else { '1.0.0' }
New-Item -ItemType Directory -Force 'dist' | Out-Null
& $iscc "/DAppVersion=$version" "/DStageDir=$stage" "/DStandaloneExe=$relativeExe" "/DHasVST2=$hasVst2" "/DInstallerFlavor=$flavor" "/O$(Join-Path (Get-Location) 'dist')" 'installer\windows\GridARV.iss'
if ($LASTEXITCODE -ne 0) { throw "Inno Setup failed with exit code $LASTEXITCODE" }
