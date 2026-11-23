Unicode True
!include "MUI2.nsh"
!include "LogicLib.nsh"

!ifndef STAGEDIR
  !error "STAGEDIR not defined"
!endif
!ifndef OUTPUTFILE
  !error "OUTPUTFILE not defined"
!endif

Name "MIXORA DJ PRO 2.5"
OutFile "${OUTPUTFILE}"
InstallDir "$LOCALAPPDATA\Programs\MIXORA DJ PRO"
RequestExecutionLevel user
SetCompressor /SOLID lzma
ShowInstDetails show
ShowUnInstDetails show

!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "Italian"

Section "Installazione" SEC_MAIN
  SetOutPath "$INSTDIR"
  File /r "${STAGEDIR}\*.*"
  WriteUninstaller "$INSTDIR\Disinstalla.exe"
  CreateDirectory "$SMPROGRAMS\MIXORA DJ PRO"
  CreateShortcut "$SMPROGRAMS\MIXORA DJ PRO\MIXORA DJ PRO.lnk" "$INSTDIR\MIXORA DJ PRO.exe"
  CreateShortcut "$SMPROGRAMS\MIXORA DJ PRO\Disinstalla.lnk" "$INSTDIR\Disinstalla.exe"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MixoraDJPro" "DisplayName" "MIXORA DJ PRO 2.5"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MixoraDJPro" "UninstallString" '"$INSTDIR\Disinstalla.exe"'
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MixoraDJPro" "InstallLocation" "$INSTDIR"
SectionEnd

Section "Disinstalla"
  Delete "$SMPROGRAMS\MIXORA DJ PRO\MIXORA DJ PRO.lnk"
  Delete "$SMPROGRAMS\MIXORA DJ PRO\Disinstalla.lnk"
  RMDir "$SMPROGRAMS\MIXORA DJ PRO"
  RMDir /r "$INSTDIR"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MixoraDJPro"
SectionEnd
