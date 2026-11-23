Unicode True
!include "MUI2.nsh"
!include "LogicLib.nsh"

!ifndef STAGEDIR
  !error "STAGEDIR not defined"
!endif
!ifndef OUTPUTFILE
  !error "OUTPUTFILE not defined"
!endif

Name "MIXORA DJ PRO 2.5.1"
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
!define MUI_FINISHPAGE_RUN "$INSTDIR\MIXORA DJ PRO.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Avvia MIXORA DJ PRO"
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "Italian"

Section "Installazione" SEC_MAIN
  SetOutPath "$INSTDIR"
  ; Put executable at installation root, fail instead of creating broken shortcuts.
  SetOverwrite on
  File "${STAGEDIR}\MIXORA DJ PRO.exe"
  File /r /x "MIXORA DJ PRO.exe" "${STAGEDIR}\*.*"
  IfFileExists "$INSTDIR\MIXORA DJ PRO.exe" executable_ok executable_missing
  executable_missing:
    MessageBox MB_ICONSTOP|MB_OK "Installazione incompleta: MIXORA DJ PRO.exe non trovato."
    Abort
  executable_ok:
  IfFileExists "$INSTDIR\Qt6Core.dll" qt_ok qt_missing
  qt_missing:
    MessageBox MB_ICONSTOP|MB_OK "Installazione incompleta: librerie Qt mancanti."
    Abort
  qt_ok:
  DetailPrint "MIXORA DJ PRO.exe e Qt6Core.dll installati correttamente."
  WriteUninstaller "$INSTDIR\Disinstalla.exe"
  CreateDirectory "$SMPROGRAMS\MIXORA DJ PRO"
  CreateShortcut "$SMPROGRAMS\MIXORA DJ PRO\MIXORA DJ PRO.lnk" "$INSTDIR\MIXORA DJ PRO.exe"
  CreateShortcut "$DESKTOP\MIXORA DJ PRO.lnk" "$INSTDIR\MIXORA DJ PRO.exe"
  CreateShortcut "$SMPROGRAMS\MIXORA DJ PRO\Disinstalla.lnk" "$INSTDIR\Disinstalla.exe"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MixoraDJPro" "DisplayName" "MIXORA DJ PRO 2.5.1"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MixoraDJPro" "UninstallString" '"$INSTDIR\Disinstalla.exe"'
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MixoraDJPro" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MixoraDJPro" "DisplayIcon" "$INSTDIR\MIXORA DJ PRO.exe"
SectionEnd

Section "Disinstalla"
  Delete "$SMPROGRAMS\MIXORA DJ PRO\MIXORA DJ PRO.lnk"
  Delete "$DESKTOP\MIXORA DJ PRO.lnk"
  Delete "$SMPROGRAMS\MIXORA DJ PRO\Disinstalla.lnk"
  RMDir "$SMPROGRAMS\MIXORA DJ PRO"
  RMDir /r "$INSTDIR"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MixoraDJPro"
SectionEnd
