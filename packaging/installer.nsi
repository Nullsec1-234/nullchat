!include "MUI2.nsh"

Name "Nullchat"
OutFile "Nullchat-Setup.exe"
InstallDir "$PROGRAMFILES64\Nullchat"
RequestExecutionLevel admin
Icon "nullchat.ico"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Section "Install"
  SetOutPath "$INSTDIR\bin"
  File /r "build\src\client\Release\*.*"
  File "build\src\server\Release\chatter-server.exe"
  File "nullchat.example.json"

  CreateDirectory "$SMPROGRAMS\Nullchat"
  CreateShortCut "$SMPROGRAMS\Nullchat\Nullchat.lnk" "$INSTDIR\bin\chatter.exe"
  CreateShortCut "$SMPROGRAMS\Nullchat\Nullchat Server.lnk" "$INSTDIR\bin\chatter-server.exe"
  CreateShortCut "$DESKTOP\Nullchat.lnk" "$INSTDIR\bin\chatter.exe"
SectionEnd

Section "Uninstall"
  RMDir /r "$INSTDIR"
  RMDir /r "$SMPROGRAMS\Nullchat"
  Delete "$DESKTOP\Nullchat.lnk"
SectionEnd
