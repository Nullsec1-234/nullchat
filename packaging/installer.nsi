!include "MUI2.nsh"

Name "Nullchat"
OutFile "Nullchat-Setup.exe"
InstallDir "$PROGRAMFILES64\Nullchat"
RequestExecutionLevel admin
Icon "packaging/nullchat.ico"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Section "Install"
  SetOutPath "$INSTDIR\bin"
  File "build/src/client/chatter.exe"
  File "build/src/server/chatter-server.exe"
  File /r "build/src/client/*.dll"
  File /r "build/src/client/platforms/"
  File /r "build/src/client/sqldrivers/"
  File /r "build/src/client/styles/"
  File /r "build/src/client/imageformats/"
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
