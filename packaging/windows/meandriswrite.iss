; Windows installer (Inno Setup 6), built by packaging/package-windows.sh, which passes
; AppVersion, SourceDir (the exe with its Qt files) and OutputDir.
; The user chooses "only for me" (no admin rights, %LOCALAPPDATA%\Programs) or "all users"
; (Program Files). No file types are registered (see the README).
; Users see the name "Meandris Write"; the install folder and file names stay "meandriswrite".

#ifndef AppVersion
  #error Build this with packaging/package-windows.sh
#endif

[Setup]
; Never change the AppId: it is how Windows recognises an older version to replace.
AppId={{F1FC098E-E3EC-4FDB-9495-C0A9600AA39F}
AppName=Meandris Write
AppVersion={#AppVersion}
AppVerName=Meandris Write {#AppVersion}
AppPublisher=Rik Onckelinx - meandris.ai
AppPublisherURL=https://github.com/onckelinx/meandriswrite
AppSupportURL=https://github.com/onckelinx/meandriswrite
AppUpdatesURL=https://github.com/onckelinx/meandriswrite/releases
VersionInfoVersion={#AppVersion}
DefaultDirName={autopf}\meandriswrite
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
; 64-bit only; Windows 10 1809 is the oldest version Qt 6.8 supports.
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.17763
UninstallDisplayName=Meandris Write
UninstallDisplayIcon={app}\meandriswrite.exe
OutputDir={#OutputDir}
OutputBaseFilename=meandriswrite-{#AppVersion}-windows-x64-setup
; Relative to this script's folder.
SetupIconFile=meandriswrite.ico
WizardStyle=modern
Compression=lzma2/max
SolidCompression=yes

[Tasks]
Name: desktopicon; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\Meandris Write"; Filename: "{app}\meandriswrite.exe"
Name: "{autodesktop}\Meandris Write"; Filename: "{app}\meandriswrite.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\meandriswrite.exe"; Description: "{cm:LaunchProgram,Meandris Write}"; Flags: nowait postinstall skipifsilent
