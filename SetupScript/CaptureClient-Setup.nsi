; CaptureClient-Setup.nsi

;--------------------------------

; The name of the installer
Name "Capture Client"

; The file to write
OutFile "CaptureClient-Setup.exe"

; The default installation directory
InstallDir $PROGRAMFILES\Capture

;--------------------------------

; Pages

;Page directory
 PageEx license
   LicenseText "GNU GENERAL PUBLIC LICENSE, v2"
   LicenseData COPYING
 PageExEnd


Page instfiles

;UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section ""
  ;Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File 7za.exe
  File CaptureClient.exe
  File Readme.txt
  File COPYING
  File *.exl
  File *.sys
  File *.conf
  File FileMonitorInstallation.inf
  ExecWait "rundll32 setupapi.dll,InstallHinfSection DefaultUninstall 0 $INSTDIR\FileMonitorInstallation.inf"
  ExecWait "rundll32 setupapi.dll,InstallHinfSection DefaultInstall 0 $INSTDIR\FileMonitorInstallation.inf"

  SetOutPath $INSTDIR\plugins
  File plugins\*.dll

  SetOutPath $INSTDIR
  WriteUninstaller "uninstall.exe"

    ; write uninstall strings
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CaptureClient" "DisplayName" "Capture Client (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CaptureClient" "UninstallString" '"$INSTDIR\uninstall.exe"'

   ; reboot
  MessageBox MB_OK "The machine needs to reboot to complete the installation. Press OK for reboot."  
  Reboot  
SectionEnd

Section "Uninstall"
  
  ; Uninstall file monitor;
  ExecWait "rundll32 setupapi.dll,InstallHinfSection DefaultUninstall 0 $INSTDIR\FileMonitorInstallation.inf"

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CaptureClient"

  ; Remove files and uninstaller
  Delete $INSTDIR\plugins\*.dll
  RMDir "$INSTDIR\plugins"
  Delete $INSTDIR\7za.exe
  Delete $INSTDIR\CaptureClient.exe
  Delete $INSTDIR\COPYING
  Delete $INSTDIR\*.exl
  Delete $INSTDIR\*.sys
  Delete $INSTDIR\Readme.txt
  Delete $INSTDIR\*.conf
  Delete $INSTDIR\FileMonitorInstallation.inf
  Delete $INSTDIR\uninstall.exe
  
  RMDir "$INSTDIR"
SectionEnd

