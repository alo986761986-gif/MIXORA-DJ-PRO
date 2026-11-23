#define MyAppName "MIXORA DJ PRO"
#define MyAppVersion "2.6.0"
#ifndef StageDir
  #error StageDir must point to the built and deployed Windows application folder.
#endif

[Setup]
AppId={{E4919021-EFD9-4BA8-AE25-247D62D5CD82}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher=MIXORA
DefaultDirName={localappdata}\Programs\MIXORA DJ PRO
DefaultGroupName=MIXORA DJ PRO
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
OutputBaseFilename=MIXORA_DJ_PRO_2_6_Setup_x64
Compression=lzma
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\MIXORA DJ PRO.exe
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
ChangesAssociations=no
CloseApplications=yes

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\MIXORA DJ PRO"; Filename: "{app}\MIXORA DJ PRO.exe"; WorkingDir: "{app}"
Name: "{autodesktop}\MIXORA DJ PRO"; Filename: "{app}\MIXORA DJ PRO.exe"; WorkingDir: "{app}"

[Run]
Filename: "{app}\MIXORA DJ PRO.exe"; Description: "Avvia MIXORA DJ PRO"; Flags: nowait postinstall skipifsilent
