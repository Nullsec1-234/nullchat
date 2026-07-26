!include "MUI2.nsh"

Name "Nullchat"
OutFile "Nullchat-Setup.exe"
InstallDir "$PROGRAMFILES64\Nullchat"
RequestExecutionLevel admin
Icon "packaging\nullchat.ico"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Section "Install"
  SetOutPath "$INSTDIR\bin"
  File "build\src\client\Release\chatter.exe"
  File "build\src\server\Release\chatter-server.exe"
  File /r "build\src\client\Release\*.dll"
  File /r "build\src\client\Release\*.qml"
  File /r "build\src\client\Release\plugins\"
  File /r "build\src\client\Release\platforms\"
  File /r "build\src\client\Release\sqldrivers\"
  File /r "build\src\client\Release\styles\"
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
