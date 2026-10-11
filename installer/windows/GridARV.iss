#define AppName "Grid ARV"
#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef StageDir
  #define StageDir "..\..\dist\stage"
#endif
#ifndef StandaloneExe
  #define StandaloneExe "Grid ARV.exe"
#endif

[Setup]
AppId={{A3D2D04E-1307-4B0D-A36D-7E8A6C6C9F50}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=Kone Labie
DefaultDirName={autopf}\Grid ARV
DefaultGroupName=Grid ARV
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
OutputBaseFilename=GRID-ARV_Installer-Windows
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=Grid ARV
Uninstallable=yes

[Types]
Name: "standard"; Description: "Standard installation"
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plug-in (for your DAW and OBS audio workflows)"; Types: standard full custom
Name: "standalone"; Description: "Standalone application"; Types: standard full custom

[Files]
Source: "{#StageDir}\VST3\*"; DestDir: "{commoncf}\VST3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#StageDir}\Standalone\*"; DestDir: "{app}\Standalone"; Components: standalone; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\Grid ARV"; Filename: "{app}\Standalone\{#StandaloneExe}"; WorkingDir: "{app}\Standalone"; Components: standalone
Name: "{autodesktop}\Grid ARV"; Filename: "{app}\Standalone\{#StandaloneExe}"; WorkingDir: "{app}\Standalone"; Tasks: desktopicon; Components: standalone

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked; Components: standalone

[Run]
Filename: "{app}\Standalone\{#StandaloneExe}"; Description: "Launch Grid ARV"; WorkingDir: "{app}\Standalone"; Flags: postinstall nowait skipifsilent; Components: standalone
