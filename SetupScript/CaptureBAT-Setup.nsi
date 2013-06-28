; CaptureBAT-Setup.nsi

;--------------------------------

; The name of the installer
Name "Capture BAT"

; The file to write
OutFile "CaptureBAT-Setup.exe"

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
  File CaptureBAT.exe
  File Readme.txt
  File COPYING
  File *.exl
  File *.sys
  File FileMonitorInstallation.inf
  ExecWait "rundll32 setupapi.dll,InstallHinfSection DefaultUninstall 0 $INSTDIR\FileMonitorInstallation.inf"
  ExecWait "rundll32 setupapi.dll,InstallHinfSection DefaultInstall 0 $INSTDIR\FileMonitorInstallation.inf"

  SetOutPath $INSTDIR
  WriteUninstaller "uninstall.exe"

    ; write uninstall strings
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CaptureBAT" "DisplayName" "Capture BAT (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CaptureBAT" "UninstallString" '"$INSTDIR\uninstall.exe"'

   ; reboot
  MessageBox MB_OK "The machine needs to reboot to complete the installation. Press OK for reboot."  
  Reboot  
SectionEnd

Section "Uninstall"
  
  ; Uninstall file monitor;
  ExecWait "rundll32 setupapi.dll,InstallHinfSection DefaultUninstall 0 $INSTDIR\FileMonitorInstallation.inf"

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CaptureBAT"

  ; Remove files and uninstaller
  Delete $INSTDIR\7za.exe
  Delete $INSTDIR\CaptureBAT.exe
  Delete $INSTDIR\COPYING
  Delete $INSTDIR\*.exl
  Delete $INSTDIR\*.sys
  Delete $INSTDIR\Readme.txt
  Delete $INSTDIR\FileMonitorInstallation.inf
  Delete $INSTDIR\uninstall.exe
  
  RMDir "$INSTDIR"
SectionEnd

