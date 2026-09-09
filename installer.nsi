Unicode true
RequestExecutionLevel admin
SetCompressor /SOLID lzma

!include "LogicLib.nsh"
!include "x64.nsh"
!include "Sections.nsh"

Var LastX86
Var LastX64

!define AppName          "DECtalk 4.99 SAPI5"
!define UninstallerName  "uninstall.exe"

!define CLSID       "{33954DF7-7F4C-4027-8022-3B3474490393}"
!define VOICES_KEY  "SOFTWARE\Microsoft\Speech\Voices\Tokens"
!define DICT_KEY    "SOFTWARE\DECtalk Software\DECtalk\4.99\US"
!define UNINST_KEY  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${AppName}"

Name    "${AppName}"
OutFile "dt499sapi5_installer.exe"
InstallDir "$PROGRAMFILES64\DECtalk 4.99 SAPI5"

Page components
Page instfiles
UninstPage instfiles


!macro RegVoice Id Display ShortName Gender Age VoiceNum
  WriteRegStr   HKLM "${VOICES_KEY}\${Id}"              ""                "${Display}"
  WriteRegStr   HKLM "${VOICES_KEY}\${Id}"              "409"             "${Display}"
  WriteRegStr   HKLM "${VOICES_KEY}\${Id}"              "CLSID"           "${CLSID}"
  WriteRegDWORD HKLM "${VOICES_KEY}\${Id}"              "Voice"           ${VoiceNum}
  WriteRegStr   HKLM "${VOICES_KEY}\${Id}\Attributes"   "Gender"          "${Gender}"
  WriteRegStr   HKLM "${VOICES_KEY}\${Id}\Attributes"   "Language"        "409"
  WriteRegStr   HKLM "${VOICES_KEY}\${Id}\Attributes"   "Age"             "${Age}"
  WriteRegStr   HKLM "${VOICES_KEY}\${Id}\Attributes"   "Vendor"          "DECtalk"
  WriteRegStr   HKLM "${VOICES_KEY}\${Id}\Attributes"   "Name"            "${ShortName}"
!macroend

!macro RegisterAllVoices
  !insertmacro RegVoice "DECtalkBetty"  "DECtalk Betty"  "Beautiful Betty"  "Female" "Adult"         1
  !insertmacro RegVoice "DECtalkDennis" "DECtalk Dennis" "Doctor Dennis" "Male"   "Adult"         4
  !insertmacro RegVoice "DECtalkFrank"  "DECtalk Frank"  "Frail Frank"  "Male"   "Senior; Adult" 3
  !insertmacro RegVoice "DECtalkHarry"  "DECtalk Harry"  "Huge Harry"  "Male"   "Adult"         2
  !insertmacro RegVoice "DECtalkKit"    "DECtalk Kit"    "Kit the Kid"    "Male"   "Child"         5
  !insertmacro RegVoice "DECtalkPaul"   "DECtalk Paul"   "Perfect Paul"   "Male"   "Adult"         0
  !insertmacro RegVoice "DECtalkRita"   "DECtalk Rita"   "Rough Rita"   "Female" "Adult"         7
  !insertmacro RegVoice "DECtalkUrsula" "DECtalk Ursula" "Uppity Ursula" "Female" "Senior; Adult" 6
  !insertmacro RegVoice "DECtalkWendy"  "DECtalk Wendy"  "Whispering Wendy"  "Female" "Adult"         8
  WriteRegStr HKLM "${VOICES_KEY}\DECtalkPaul\Attributes" "VendorPreferred" ""
!macroend

!macro UnregisterAllVoices
  Push $0
  Push $1
  Push $2
  StrCpy $0 0
  ${Do}
    EnumRegKey $1 HKLM "${VOICES_KEY}" $0
    ${If} $1 == ""
      ${ExitDo}
    ${EndIf}
    ReadRegStr $2 HKLM "${VOICES_KEY}\$1" "CLSID"
    ${If} $2 == "${CLSID}"
      DeleteRegKey HKLM "${VOICES_KEY}\$1"
    ${Else}
      IntOp $0 $0 + 1
    ${EndIf}
  ${Loop}
  Pop $2
  Pop $1
  Pop $0
!macroend

!macro RegisterDict
  WriteRegStr HKLM "${DICT_KEY}" "MainDict" "$INSTDIR\dtalk_us.dic"
!macroend

!macro UnregisterDict
  DeleteRegValue HKLM "${DICT_KEY}" "MainDict"
  DeleteRegKey /ifempty HKLM "${DICT_KEY}"
  DeleteRegKey /ifempty HKLM "SOFTWARE\DECtalk Software\DECtalk\4.99"
  DeleteRegKey /ifempty HKLM "SOFTWARE\DECtalk Software\DECtalk"
  DeleteRegKey /ifempty HKLM "SOFTWARE\DECtalk Software"
!macroend


Section -"Dictionary"
  SetOutPath $INSTDIR
  File "dist\dtalk_us.dic"
SectionEnd

Section "X86" SecX86
  SetOutPath $INSTDIR
  File "dist\ttseng.dll"
  ExecWait '"$SYSDIR\regsvr32.exe" /s "$INSTDIR\ttseng.dll"' $0
  ${If} $0 <> 0
    MessageBox MB_OK|MB_ICONEXCLAMATION "Couldn't register ttseng.dll (regsvr32 returned $0)."
  ${EndIf}
  SetRegView 32
  !insertmacro RegisterAllVoices
  !insertmacro RegisterDict
  SetRegView Default
SectionEnd

Section "X64" SecX64
  SetOutPath $INSTDIR
  File "dist\ttseng64.dll"
  ${DisableX64FSRedirection}
  ExecWait '"$SYSDIR\regsvr32.exe" /s "$INSTDIR\ttseng64.dll"' $0
  ${EnableX64FSRedirection}
  ${If} $0 <> 0
    MessageBox MB_OK|MB_ICONEXCLAMATION "Couldn't register ttseng64.dll (regsvr32 returned $0)."
  ${EndIf}
  SetRegView 64
  !insertmacro RegisterAllVoices
  !insertmacro RegisterDict
  SetRegView Default
SectionEnd

Section "DECtalk Voice Manager"
  SetOutPath $INSTDIR
  File "dist\dtvoicemgr.exe"
CreateShortcut "$DESKTOP\DECtalk 4.99 Voice Manager.lnk" "$INSTDIR\dtvoicemgr.exe"
SectionEnd

Section -"MainSection"
  WriteRegStr HKLM "${UNINST_KEY}" "DisplayName"     "${AppName}"
  WriteRegStr HKLM "${UNINST_KEY}" "DisplayVersion"  "4.99"
  WriteRegStr HKLM "${UNINST_KEY}" "Publisher"       "DECtalk Software"
  WriteRegStr HKLM "${UNINST_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${UNINST_KEY}" "DisplayIcon"     "$INSTDIR\${UninstallerName}"
  WriteRegStr HKLM "${UNINST_KEY}" "UninstallString" '"$INSTDIR\${UninstallerName}"'
  WriteRegStr HKLM "${UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\${UninstallerName}" /S'
  WriteRegDWORD HKLM "${UNINST_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${UNINST_KEY}" "NoRepair" 1
  WriteUninstaller "$INSTDIR\${UninstallerName}"
SectionEnd


Function .onInit
  SetShellVarContext all
  ${IfNot} ${RunningX64}
    SectionSetFlags ${SecX64} 0
    SectionSetText  ${SecX64} ""
  ${EndIf}
  StrCpy $LastX86 ${SF_SELECTED}
  StrCpy $LastX64 0
  ${If} ${RunningX64}
    StrCpy $LastX64 ${SF_SELECTED}
  ${EndIf}
FunctionEnd


Function .onSelChange
  Push $0
  Push $1
  Push $2
  SectionGetFlags ${SecX86} $0
  IntOp $0 $0 & ${SF_SELECTED}
  SectionGetFlags ${SecX64} $1
  IntOp $1 $1 & ${SF_SELECTED}
  IntOp $2 $0 + $1
  ${If} $2 == 0
    ${If} $LastX86 <> 0
      !insertmacro SelectSection ${SecX86}
    ${Else}
      !insertmacro SelectSection ${SecX64}
    ${EndIf}
    SectionGetFlags ${SecX86} $0
    IntOp $0 $0 & ${SF_SELECTED}
    SectionGetFlags ${SecX64} $1
    IntOp $1 $1 & ${SF_SELECTED}
  ${EndIf}
  StrCpy $LastX86 $0
  StrCpy $LastX64 $1
  Pop $2
  Pop $1
  Pop $0
FunctionEnd


Function un.onInit
  SetShellVarContext all
  MessageBox MB_YESNO|MB_ICONQUESTION \
    "Are you sure you want to remove ${AppName} from your computer?" IDYES +2
  Quit
FunctionEnd


Section "Uninstall"
  ${If} ${RunningX64}
    ${If} ${FileExists} "$INSTDIR\ttseng64.dll"
      ${DisableX64FSRedirection}
      ExecWait '"$SYSDIR\regsvr32.exe" /s /u "$INSTDIR\ttseng64.dll"'
      ${EnableX64FSRedirection}
    ${EndIf}
    SetRegView 64
    !insertmacro UnregisterAllVoices
    !insertmacro UnregisterDict
    SetRegView Default
  ${EndIf}

  ${If} ${FileExists} "$INSTDIR\ttseng.dll"
    ExecWait '"$SYSDIR\regsvr32.exe" /s /u "$INSTDIR\ttseng.dll"'
  ${EndIf}
  SetRegView 32
  !insertmacro UnregisterAllVoices
  !insertmacro UnregisterDict
  SetRegView Default

  DeleteRegKey HKLM "${UNINST_KEY}"

  Delete "$DESKTOP\DECtalk 4.99 Voice Manager.lnk"
  Delete "$INSTDIR\dtvoicemgr.exe"
  Delete "$INSTDIR\ttseng.dll"
  Delete "$INSTDIR\ttseng64.dll"
  Delete "$INSTDIR\dtalk_us.dic"
  Delete "$INSTDIR\${UninstallerName}"
  RMDir  "$INSTDIR"
SectionEnd
