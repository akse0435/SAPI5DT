@echo off
setlocal enableextensions
title Build SAPI5 DECtalk - Win32 + x64 (Visual Studio 2022)

rem Run from the SAPI5DT folder with VS2022 C++ and the v143 ATL component installed.

set "SAPIDIR=%~dp0"
if "%SAPIDIR:~-1%"=="\" set "SAPIDIR=%SAPIDIR:~0,-1%"
for %%i in ("%SAPIDIR%\..") do set "ROOT=%%~fi"
pushd "%SAPIDIR%"

echo [1/7] Visual Studio 2022
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (echo ERROR: vswhere.exe not found - is VS2022 installed? & goto :fail)
set "VSDIR="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR (echo ERROR: no VS installation with C++ build tools found. & goto :fail)
if not exist "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" (echo ERROR: vcvars64.bat missing in "%VSDIR%". & goto :fail)
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 goto :fail
rem vcvars64 exports Platform=x64; clear it so msbuild honours /p:Platform.
set "Platform="
where cl.exe      >nul 2>&1 || (echo ERROR: cl.exe not found after vcvars64.      & goto :fail)
where link.exe    >nul 2>&1 || (echo ERROR: link.exe not found after vcvars64.    & goto :fail)
where midl.exe    >nul 2>&1 || (echo ERROR: midl.exe not found after vcvars64.    & goto :fail)
where rc.exe      >nul 2>&1 || (echo ERROR: rc.exe not found after vcvars64.      & goto :fail)
where msbuild.exe >nul 2>&1 || (echo ERROR: msbuild.exe not found after vcvars64. & goto :fail)
set "ATLOK="
for %%d in ("%INCLUDE:;=" "%") do if exist "%%~d\atlbase.h" set "ATLOK=1"
if not defined ATLOK (echo ERROR: atlbase.h not found - install the "C++ ATL for latest v143 build tools" component. & goto :fail)

echo [2/7] Locating the DECtalk repo
if not exist "%SAPIDIR%\ttseng.dsp" (echo ERROR: run this script from the SAPI5DT folder. & goto :fail)
if not exist "%ROOT%\src\dapi\src\DECtalk API.vcxproj" (echo ERROR: SAPI5DT must sit inside the DECtalk repo. & goto :fail)

set "DAPISRC=%ROOT%\src\dapi\src"
set "DTLIBDIR=%ROOT%\src\dapi\build\dtstatic_v143"
set "DICDIR=%ROOT%\src\dapi\build\dic\x64\Release - ENGLISH_US"
set "DICTXT=%DAPISRC%\dic\Dic_us.txt"
set "DIST=%SAPIDIR%\dist"
set "BUILD=%SAPIDIR%\build"
set "STAGE=%BUILD%\ttseng"
if not exist "%DIST%" mkdir "%DIST%"
if not exist "%BUILD%" mkdir "%BUILD%"

echo [3/7] Patching sources and generating project files
rem Every source this step patches gets a .orig64 backup and is restored on each run.
powershell -NoProfile -ExecutionPolicy Bypass -Command "$L=[System.IO.File]::ReadAllLines('%~f0'); $o=@(); foreach($l in $L){ if($l.StartsWith(':::')){ $o += $l.Substring(3) } }; [System.IO.File]::WriteAllLines('%BUILD%\_prepare.ps1', $o)"
if errorlevel 1 goto :fail
powershell -NoProfile -ExecutionPolicy Bypass -File "%BUILD%\_prepare.ps1" -Root "%ROOT%" -SapiDir "%SAPIDIR%"
if errorlevel 1 goto :fail

echo [4/7] DECtalk static libraries
rem Keep MaxSpeed: Full exposes a latent race on async_change in kernel.h.
msbuild "%DAPISRC%\DECtalkStatic.vcxproj" /nologo /m /v:m /p:Configuration=Release /p:Platform=Win32
if errorlevel 1 (echo ERROR: DECtalkStatic.lib Win32 failed. & goto :fail)
msbuild "%DAPISRC%\DECtalkStatic.vcxproj" /nologo /m /v:m /p:Configuration=Release /p:Platform=x64
if errorlevel 1 (echo ERROR: DECtalkStatic.lib x64 failed. & goto :fail)
if not exist "%DTLIBDIR%\Win32\Release\DECtalkStatic.lib" (echo ERROR: Win32 DECtalkStatic.lib was not produced. & goto :fail)
if not exist "%DTLIBDIR%\x64\Release\DECtalkStatic.lib" (echo ERROR: x64 DECtalkStatic.lib was not produced. & goto :fail)

echo [5/7] Dictionary
rem One x64 pass is enough because the .dic format is architecture neutral.
msbuild "%DAPISRC%\Internal Dictionary Compiler.vcxproj" /nologo /m /v:m /p:Configuration="Release - ENGLISH_US" /p:Platform=x64 /p:PostBuildEventUseInBuild=false
if errorlevel 1 (echo ERROR: dictionary compiler build failed. & goto :fail)
if not exist "%DICDIR%\Internal Dictionary Compiler.exe" (echo ERROR: dictionary compiler was not produced. & goto :fail)
"%DICDIR%\Internal Dictionary Compiler.exe" "%DICTXT%" "%DIST%\dtalk_us.dic" /t:win32
if errorlevel 1 (echo ERROR: dictionary compilation failed. & goto :fail)
if not exist "%DIST%\dtalk_us.dic" (echo ERROR: dtalk_us.dic was not produced. & goto :fail)

echo [6/7] ttseng.dll + ttseng64.dll
msbuild "%STAGE%\TtsEng.vcxproj" /nologo /m /v:m /p:Configuration=Release /p:Platform=Win32
if errorlevel 1 (echo ERROR: ttseng.dll failed. & goto :fail)
msbuild "%STAGE%\TtsEng.vcxproj" /nologo /m /v:m /p:Configuration=Release /p:Platform=x64
if errorlevel 1 (echo ERROR: ttseng64.dll failed. & goto :fail)
if not exist "%DIST%\ttseng.dll" (echo ERROR: ttseng.dll was not produced. & goto :fail)
if not exist "%DIST%\ttseng64.dll" (echo ERROR: ttseng64.dll was not produced. & goto :fail)

echo [7/7] dtvoicemgr.exe
rem Deliberately Win32: a 32-bit build can still preview through ttseng.dll.
msbuild "%BUILD%\dtvoicemgr\DtVoiceMgr.vcxproj" /nologo /m /v:m /p:Configuration=Release /p:Platform=Win32
if errorlevel 1 (echo ERROR: voice manager build failed. & goto :fail)
if not exist "%DIST%\dtvoicemgr.exe" (echo ERROR: dtvoicemgr.exe was not produced. & goto :fail)

echo.
echo DONE.
echo   %DIST%\ttseng.dll
echo   %DIST%\ttseng64.dll
echo   %DIST%\dtvoicemgr.exe
echo   %DIST%\dtalk_us.dic
popd
endlocal
exit /b 0

:fail
echo.
echo *** BUILD ABORTED ***
echo Full log:  "%~nx0" ^> "%SAPIDIR%\log64.txt" 2^>^&1
popd
endlocal
exit /b 1

rem Lines starting with three colons become build\_prepare.ps1 and cannot contain a percent sign.
:::param([Parameter(Mandatory=$true)][string]$Root,
:::      [Parameter(Mandatory=$true)][string]$SapiDir)
:::$ErrorActionPreference = 'Stop'
:::
::: # Latin-1 round-trips bytes 0-255, so sources stay byte exact.
:::$enc = [System.Text.Encoding]::GetEncoding(28591)
:::function Read-Src([string]$p) { [System.IO.File]::ReadAllText($p, $enc) }
:::function Write-Src([string]$p, [string]$s) { [System.IO.File]::WriteAllText($p, $s, $enc) }
:::
:::$dapi  = Join-Path $Root 'src\dapi\src'
:::$stage = Join-Path $SapiDir 'build\ttseng'
:::
::: # LLP64 fix of DECtalk's callback ABI, a no-op on Win32 where LONG_PTR is LONG.
:::$typedefBlock = @'
:::/* --- DECTALK_LLP64_CALLBACK_TYPES (auto-inserted by build_sapi5dt64.bat) --- */
:::#ifndef DECTALK_LLP64_CALLBACK_TYPES
:::#define DECTALK_LLP64_CALLBACK_TYPES
:::#if defined(_WIN32) || defined(WIN32) || defined(_WIN64)
:::#include <basetsd.h>
:::typedef LONG_PTR  DT_LPARAM_T;
:::typedef DWORD_PTR DT_UPARAM_T;
:::#else
:::typedef long          DT_LPARAM_T;
:::typedef unsigned long DT_UPARAM_T;
:::#endif
:::#endif
:::
:::'@
:::
:::function Patch-Callback([string]$path, [bool]$inject, [bool]$backup) {
:::  if (-not (Test-Path -LiteralPath $path)) { Write-Host ('  missing: ' + $path); return }
:::  if ($backup) {
:::    $b = "$path.orig64"
:::    if (Test-Path -LiteralPath $b) { Copy-Item -LiteralPath $b -Destination $path -Force }
:::    else { Copy-Item -LiteralPath $path -Destination $b }
:::  }
:::  $s = Read-Src $path
:::  # (a) the callback tuple: VOID (*DtCallbackRoutine)(LONG,LONG,DWORD,UINT)
:::  $s = $s -replace '\(\s*LONG\s*,\s*LONG\s*,\s*DWORD\s*,\s*UINT\s*\)', '(DT_LPARAM_T,DT_LPARAM_T,DT_UPARAM_T,UINT)'
:::  # (b) the instance parameter right after the callback tuple
:::  $s = $s -replace '(DT_UPARAM_T,UINT\)\s*,\s*)LONG\b', '$1DT_UPARAM_T'
:::  # (c) the field in TTS_HANDLE_T
:::  $s = $s -replace '\bDWORD\s+dwTTSInstanceParameter\s*;', 'DT_UPARAM_T dwTTSInstanceParameter;'
:::  # (d) Report_TTS_Status
:::  $s = $s -replace 'UINT\s+uiMsg\s*,\s*long\s+lParam1\s*,\s*long\s+lParam2', 'UINT uiMsg, DT_LPARAM_T lParam1, DT_LPARAM_T lParam2'
:::  # (e) callbacks with named parameters: DefaultTTSCallbackRoutine and TTSCallbackRoutine
:::  $s = $s -replace 'LONG\s+lParam1\s*,\s*LONG\s+lParam2\s*,\s*DWORD\s+dwInstanceParam\s*,\s*UINT\s+uiMsg', 'DT_LPARAM_T lParam1, DT_LPARAM_T lParam2, DT_UPARAM_T dwInstanceParam, UINT uiMsg'
:::  # (f) TextToSpeechStartup stores hWnd in the instance parameter - must not truncate
:::  $s = $s -replace '\(\s*LONG\s*\)\s*hWnd', '(DT_UPARAM_T)hWnd'
:::  # (g) clean up C4047
:::  $s = $s -replace 'SetWindowLongPtr\(hWnd,\s*GWLP_USERDATA,\s*phTTS\)', 'SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)phTTS)'
:::  if ($inject -and ($s -notmatch 'DECTALK_LLP64_CALLBACK_TYPES')) { $s = $typedefBlock + $s }
:::  Write-Src $path $s
:::}
:::
:::Patch-Callback (Join-Path $dapi 'api\tts.h')    $true  $true
:::Patch-Callback (Join-Path $dapi 'api\ttsapi.h') $true  $true
:::Patch-Callback (Join-Path $dapi 'api\ttsapi.c') $false $true
:::Patch-Callback (Join-Path $Root 'src\samples\speak\Speak.c') $false $true
:::
::: # The source list comes from "DECtalk API.vcxproj" so phprint.c is included.
:::[xml]$orig = Get-Content -LiteralPath (Join-Path $dapi 'DECtalk API.vcxproj')
:::$srcFiles = @()
:::foreach ($ig in $orig.Project.ItemGroup) {
:::  foreach ($cc in $ig.ClCompile) { if ($cc -and $cc.Include) { $srcFiles += $cc.Include } }
:::}
:::if ($srcFiles.Count -lt 50) { throw "Only found $($srcFiles.Count) source files in DECtalk API.vcxproj" }
:::$ccItems = ($srcFiles | ForEach-Object { '    <ClCompile Include="' + $_ + '" />' }) -join "`r`n"
:::
:::$libProj = @'
:::<?xml version="1.0" encoding="utf-8"?>
:::<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
:::  <ItemGroup Label="ProjectConfigurations">
:::    <ProjectConfiguration Include="Release|Win32">
:::      <Configuration>Release</Configuration>
:::      <Platform>Win32</Platform>
:::    </ProjectConfiguration>
:::    <ProjectConfiguration Include="Release|x64">
:::      <Configuration>Release</Configuration>
:::      <Platform>x64</Platform>
:::    </ProjectConfiguration>
:::  </ItemGroup>
:::  <PropertyGroup Label="Globals">
:::    <VCProjectVersion>17.0</VCProjectVersion>
:::    <ProjectGuid>{A1B2C3D4-64B1-4E64-9C11-DEC7A1640001}</ProjectGuid>
:::    <RootNamespace>DECtalkStatic</RootNamespace>
:::    <Keyword>Win32Proj</Keyword>
:::  </PropertyGroup>
:::  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
:::  <PropertyGroup Label="Configuration">
:::    <ConfigurationType>StaticLibrary</ConfigurationType>
:::    <UseDebugLibraries>false</UseDebugLibraries>
:::    <PlatformToolset>v143</PlatformToolset>
:::    <CharacterSet>MultiByte</CharacterSet>
:::    <WholeProgramOptimization>false</WholeProgramOptimization>
:::  </PropertyGroup>
:::  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
:::  <PropertyGroup>
:::    <OutDir>..\build\dtstatic_v143\$(Platform)\Release\</OutDir>
:::    <IntDir>..\build\dtstatic_v143\$(Platform)\Release\obj\</IntDir>
:::    <TargetName>DECtalkStatic</TargetName>
:::  </PropertyGroup>
:::  <ItemDefinitionGroup>
:::    <ClCompile>
:::      <PreprocessorDefinitions>NDEBUG;WIN32;_WINDOWS;i386;BLD_DECTALK_DLL;ENGLISH_US;ENGLISH;ACNA;STATIC_BUILD;_CRT_SECURE_NO_WARNINGS;_CRT_NONSTDC_NO_DEPRECATE;_WINSOCK_DEPRECATED_NO_WARNINGS</PreprocessorDefinitions>
:::      <AdditionalIncludeDirectories>api;acna;cmd;lts;ph;vtm;kernel;nt;include;protos;../..;../hlsyn;hlsyn</AdditionalIncludeDirectories>
:::      <Optimization>MaxSpeed</Optimization>
:::      <FavorSizeOrSpeed>Speed</FavorSizeOrSpeed>
:::      <ExceptionHandling>Sync</ExceptionHandling>
:::      <RuntimeLibrary>MultiThreaded</RuntimeLibrary>
:::      <WarningLevel>Level3</WarningLevel>
:::      <TreatWarningAsError>false</TreatWarningAsError>
:::      <ConformanceMode>false</ConformanceMode>
:::      <SDLCheck>false</SDLCheck>
:::      <MultiProcessorCompilation>true</MultiProcessorCompilation>
:::      <MinimalRebuild>false</MinimalRebuild>
:::      <DebugInformationFormat>None</DebugInformationFormat>
:::      <DisableSpecificWarnings>4996;4244;4267;4311;4312;4302;4090;4013;4024;4133</DisableSpecificWarnings>
:::    </ClCompile>
:::    <Lib><SuppressStartupBanner>true</SuppressStartupBanner></Lib>
:::  </ItemDefinitionGroup>
:::  <ItemGroup>
:::__SOURCES__
:::  </ItemGroup>
:::  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
:::</Project>
:::'@
:::
:::Write-Src (Join-Path $dapi 'DECtalkStatic.vcxproj') $libProj.Replace('__SOURCES__', $ccItems)
:::
::: # Patch copies in build\ttseng so the originals and the VS6 build stay untouched.
:::if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
:::New-Item -ItemType Directory -Path $stage | Out-Null
:::
:::$names = @('SMITS5.C','stdafx.cpp','stdafx.h','ttsapi.h','ttseng.cpp','ttseng.def','ttseng.idl','ttseng.rc','ttsengobj.cpp','ttsengobj.h','ttsengver.h','ttsengobj.rgs','resource.h','version.rc2')
:::foreach ($n in $names) {
:::  $p = Join-Path $SapiDir $n
:::  if (Test-Path -LiteralPath $p) { Copy-Item -LiteralPath $p -Destination (Join-Path $stage $n) }
:::  else { Write-Host ('  missing: ' + $n) }
:::}
:::
::: # The local ttsapi.h gets the same LLP64 treatment.
:::Patch-Callback (Join-Path $stage 'ttsapi.h') $true $false
:::
::: # 3b) callback signature and pointer cast in the engine itself
:::$p = Join-Path $stage 'ttsengobj.h'
:::$s = Read-Src $p
:::$s = $s -replace 'LONG\s+LParam1\s*,\s*LONG\s+lParam2\s*,\s*DWORD\s+user\s*,\s*UINT\s+msg', 'DT_LPARAM_T LParam1, DT_LPARAM_T lParam2, DT_UPARAM_T user, UINT msg'
:::Write-Src $p $s
:::
:::$p = Join-Path $stage 'ttsengobj.cpp'
:::$s = Read-Src $p
:::$s = $s -replace 'LONG\s+LParam1\s*,\s*LONG\s+lParam2\s*,\s*DWORD\s+user\s*,\s*UINT\s+msg', 'DT_LPARAM_T LParam1, DT_LPARAM_T lParam2, DT_UPARAM_T user, UINT msg'
:::$s = $s -replace '\(\s*LONG\s*\)\s*this', '(DT_UPARAM_T)this'
:::Write-Src $p $s
:::
::: # 3c) SMITS5.C: 0xFFFFFFFF is not a valid HANDLE on x64
:::$p = Join-Path $stage 'SMITS5.C'
:::$s = Read-Src $p
:::$s = $s -replace '\(\s*HANDLE\s*\)\s*0xFFFFFFFF', 'INVALID_HANDLE_VALUE'
:::Write-Src $p $s
:::
::: # 3d) modern ATL has neither atlimpl.cpp nor statreg.cpp
:::$p = Join-Path $stage 'stdafx.cpp'
:::$s = Read-Src $p
:::$s = $s -replace '(?s)#ifdef\s+_ATL_STATIC_REGISTRY.*?#include\s*<atlimpl\.cpp>', "#if _MSC_VER < 1300`r`n#ifdef _ATL_STATIC_REGISTRY`r`n#include <statreg.h>`r`n#include <statreg.cpp>`r`n#endif`r`n#include <atlimpl.cpp>`r`n#endif"
:::Write-Src $p $s
:::
:::$p = Join-Path $stage 'stdafx.h'
:::$s = Read-Src $p
:::$s = $s -replace '#define\s+_WIN32_WINNT\s+0x0400', '#define _WIN32_WINNT 0x0601'
:::Write-Src $p $s
:::
::: # The 2001 SAPI 5.1 headers do not compile with v143, so patch staged copies.
:::$inc = Join-Path $stage 'Include'
:::New-Item -ItemType Directory -Path $inc | Out-Null
:::Copy-Item -Path (Join-Path $SapiDir 'Include\*') -Destination $inc -Force
:::
:::$p = Join-Path $inc 'sphelper.h'
:::$s = Read-Src $p
::: # C4430 (769): const without a type specifier - VC6 assumed implicit int
:::$s = $s -replace 'const\s+ulLenVendorPreferred\s*=\s*wcslen\(pszVendorPreferred\);', 'const ULONG ulLenVendorPreferred = (ULONG)wcslen(pszVendorPreferred);'
::: # C4430 (1418): function without a return type
:::$s = $s -replace 'static\s+CoMemCopyWFEX\(', 'static HRESULT CoMemCopyWFEX('
::: # C2065 (2373): VC6 let the for-loop variable outlive the loop
:::$s = $s -replace 'for \(const WCHAR \* psz = \(const WCHAR \*\)lParam; \*psz; psz\+\+\) \{\}', "const WCHAR * psz = (const WCHAR *)lParam;`r`n        for (; *psz; psz++) {}"
::: # C2440 (2559) + C2664 (2633): SPPHONEID is unsigned short, WCHAR is now native wchar_t
:::$s = $s -replace 'SPPHONEID\* pphoneId = dsPhoneId;', 'SPPHONEID* pphoneId = (SPPHONEID*)(WCHAR*)dsPhoneId;'
:::$s = $s -replace 'wcslen\(pphoneId\)', 'wcslen((const WCHAR*)pphoneId)'
:::Write-Src $p $s
:::
:::$p = Join-Path $inc 'spcollec.h'
:::$s = Read-Src $p
::: # C2061 (759/1227/1285): dependent type names in template return types need typename
:::$s = $s -replace '(?m)^CSPList<TYPE, ARG_TYPE>::CNode\*', 'typename CSPList<TYPE, ARG_TYPE>::CNode*'
:::$s = $s -replace '(?m)^CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::CAssoc\*', 'typename CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::CAssoc*'
:::Write-Src $p $s
:::
::: # The only GUID symbols the engine links against, so no sapi.lib is needed.
:::$guids = @'
:::#include <windows.h>
:::#include <initguid.h>
:::DEFINE_GUID(SPDFID_Text,         0x7ceef9f9, 0x3d13, 0x11d2, 0x9e, 0xe7, 0x00, 0xc0, 0x4f, 0x79, 0x73, 0x96);
:::DEFINE_GUID(SPDFID_WaveFormatEx, 0xc31adbae, 0x527f, 0x4ff5, 0xa2, 0x30, 0xf6, 0x2b, 0xb6, 0x1f, 0xf7, 0x0c);
:::'@
:::Write-Src (Join-Path $stage 'sapiguids.c') $guids
:::
::: # Win32 produces ttseng.dll and x64 produces ttseng64.dll, both into SAPI5DT\dist.
:::$engProj = @'
:::<?xml version="1.0" encoding="utf-8"?>
:::<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
:::  <ItemGroup Label="ProjectConfigurations">
:::    <ProjectConfiguration Include="Release|Win32">
:::      <Configuration>Release</Configuration>
:::      <Platform>Win32</Platform>
:::    </ProjectConfiguration>
:::    <ProjectConfiguration Include="Release|x64">
:::      <Configuration>Release</Configuration>
:::      <Platform>x64</Platform>
:::    </ProjectConfiguration>
:::  </ItemGroup>
:::  <PropertyGroup Label="Globals">
:::    <VCProjectVersion>17.0</VCProjectVersion>
:::    <ProjectGuid>{B2C3D4E5-64B1-4E64-9C11-DEC7A1640002}</ProjectGuid>
:::    <RootNamespace>TtsEng</RootNamespace>
:::  </PropertyGroup>
:::  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
:::  <PropertyGroup Label="Configuration">
:::    <ConfigurationType>DynamicLibrary</ConfigurationType>
:::    <UseDebugLibraries>false</UseDebugLibraries>
:::    <PlatformToolset>v143</PlatformToolset>
:::    <CharacterSet>MultiByte</CharacterSet>
:::    <UseOfAtl>Static</UseOfAtl>
:::    <WholeProgramOptimization>false</WholeProgramOptimization>
:::  </PropertyGroup>
:::  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
:::  <PropertyGroup>
:::    <OutDir>$(ProjectDir)..\..\dist\</OutDir>
:::    <IntDir>$(ProjectDir)obj\$(Platform)\</IntDir>
:::    <TargetName Condition="'$(Platform)'=='Win32'">ttseng</TargetName>
:::    <TargetName Condition="'$(Platform)'=='x64'">ttseng64</TargetName>
:::  </PropertyGroup>
:::  <ItemDefinitionGroup>
:::    <Midl>
:::      <AdditionalIncludeDirectories>$(ProjectDir)..\..\IDL</AdditionalIncludeDirectories>
:::      <OutputDirectory>$(IntDir)</OutputDirectory>
:::      <HeaderFileName>TtsEng.h</HeaderFileName>
:::      <InterfaceIdentifierFileName>TtsEng_i.c</InterfaceIdentifierFileName>
:::      <ProxyFileName>TtsEng_p.c</ProxyFileName>
:::      <DllDataFileName>dlldata.c</DllDataFileName>
:::      <TypeLibraryName>$(IntDir)TtsEng.tlb</TypeLibraryName>
:::      <TargetEnvironment Condition="'$(Platform)'=='Win32'">Win32</TargetEnvironment>
:::      <TargetEnvironment Condition="'$(Platform)'=='x64'">X64</TargetEnvironment>
:::      <MkTypLibCompatible>false</MkTypLibCompatible>
:::      <PreprocessorDefinitions>NDEBUG</PreprocessorDefinitions>
:::      <SuppressStartupBanner>true</SuppressStartupBanner>
:::    </Midl>
:::    <ClCompile>
:::      <PreprocessorDefinitions>NDEBUG;WIN32;_WINDOWS;_MBCS;_USRDLL;_ATL_STATIC_REGISTRY;_CRT_SECURE_NO_WARNINGS;_WINSOCK_DEPRECATED_NO_WARNINGS</PreprocessorDefinitions>
:::      <AdditionalIncludeDirectories>$(ProjectDir);$(IntDir);$(ProjectDir)Include</AdditionalIncludeDirectories>
:::      <Optimization>MinSpace</Optimization>
:::      <RuntimeLibrary>MultiThreaded</RuntimeLibrary>
:::      <PrecompiledHeader>NotUsing</PrecompiledHeader>
:::      <WarningLevel>Level3</WarningLevel>
:::      <ConformanceMode>false</ConformanceMode>
:::      <SDLCheck>false</SDLCheck>
:::      <TreatWarningAsError>false</TreatWarningAsError>
:::      <DebugInformationFormat>None</DebugInformationFormat>
:::      <DisableSpecificWarnings>4996;4244;4267;4311;4312;4302</DisableSpecificWarnings>
:::    </ClCompile>
:::    <ResourceCompile>
:::      <AdditionalIncludeDirectories>$(ProjectDir);$(IntDir);$(ProjectDir)Include</AdditionalIncludeDirectories>
:::      <PreprocessorDefinitions>NDEBUG</PreprocessorDefinitions>
:::    </ResourceCompile>
:::    <Link>
:::      <SubSystem>Windows</SubSystem>
:::      <ModuleDefinitionFile>$(ProjectDir)ttseng.def</ModuleDefinitionFile>
:::      <ImportLibrary>$(IntDir)$(TargetName).lib</ImportLibrary>
:::      <AdditionalDependencies>$(ProjectDir)..\..\..\src\dapi\build\dtstatic_v143\$(Platform)\Release\DECtalkStatic.lib;atls.lib;kernel32.lib;user32.lib;gdi32.lib;winspool.lib;comdlg32.lib;advapi32.lib;shell32.lib;ole32.lib;oleaut32.lib;uuid.lib;winmm.lib</AdditionalDependencies>
:::      <GenerateDebugInformation>false</GenerateDebugInformation>
:::    </Link>
:::  </ItemDefinitionGroup>
:::  <ItemGroup>
:::    <Midl Include="ttseng.idl" />
:::  </ItemGroup>
:::  <ItemGroup>
:::    <ClCompile Include="ttseng.cpp" />
:::    <ClCompile Include="ttsengobj.cpp" />
:::    <ClCompile Include="sapiguids.c" />
:::  </ItemGroup>
:::  <ItemGroup>
:::    <ResourceCompile Include="ttseng.rc" />
:::  </ItemGroup>
:::  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
:::</Project>
:::'@
:::
:::Write-Src (Join-Path $stage 'TtsEng.vcxproj') $engProj
:::
::: # The voice manager is built Win32 so it can preview through the 32-bit engine.
:::$mgrDir = Join-Path $SapiDir 'build\dtvoicemgr'
:::if (-not (Test-Path -LiteralPath $mgrDir)) { New-Item -ItemType Directory -Path $mgrDir | Out-Null }
:::
:::$mgrProj = @'
:::<?xml version="1.0" encoding="utf-8"?>
:::<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
:::  <ItemGroup Label="ProjectConfigurations">
:::    <ProjectConfiguration Include="Release|Win32">
:::      <Configuration>Release</Configuration>
:::      <Platform>Win32</Platform>
:::    </ProjectConfiguration>
:::  </ItemGroup>
:::  <PropertyGroup Label="Globals">
:::    <VCProjectVersion>17.0</VCProjectVersion>
:::    <ProjectGuid>{C3D4E5F6-64B1-4E64-9C11-DEC7A1640003}</ProjectGuid>
:::    <RootNamespace>DtVoiceMgr</RootNamespace>
:::    <Keyword>Win32Proj</Keyword>
:::  </PropertyGroup>
:::  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
:::  <PropertyGroup Label="Configuration">
:::    <ConfigurationType>Application</ConfigurationType>
:::    <UseDebugLibraries>false</UseDebugLibraries>
:::    <PlatformToolset>v143</PlatformToolset>
:::    <CharacterSet>MultiByte</CharacterSet>
:::    <WholeProgramOptimization>false</WholeProgramOptimization>
:::  </PropertyGroup>
:::  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
:::  <PropertyGroup>
:::    <OutDir>$(ProjectDir)..\..\dist\</OutDir>
:::    <IntDir>$(ProjectDir)obj\$(Platform)\</IntDir>
:::    <TargetName>dtvoicemgr</TargetName>
:::  </PropertyGroup>
:::  <ItemDefinitionGroup>
:::    <ClCompile>
:::      <PreprocessorDefinitions>NDEBUG;WIN32;_WINDOWS;_MBCS;_CRT_SECURE_NO_WARNINGS</PreprocessorDefinitions>
:::      <Optimization>MinSpace</Optimization>
:::      <RuntimeLibrary>MultiThreaded</RuntimeLibrary>
:::      <PrecompiledHeader>NotUsing</PrecompiledHeader>
:::      <WarningLevel>Level3</WarningLevel>
:::      <ConformanceMode>false</ConformanceMode>
:::      <SDLCheck>false</SDLCheck>
:::      <TreatWarningAsError>false</TreatWarningAsError>
:::      <DebugInformationFormat>None</DebugInformationFormat>
:::    </ClCompile>
:::    <Link>
:::      <SubSystem>Windows</SubSystem>
:::      <GenerateDebugInformation>false</GenerateDebugInformation>
:::      <AdditionalDependencies>kernel32.lib;user32.lib;gdi32.lib;comctl32.lib;comdlg32.lib;advapi32.lib;shell32.lib;ole32.lib;oleaut32.lib;uuid.lib</AdditionalDependencies>
:::    </Link>
:::  </ItemDefinitionGroup>
:::  <ItemGroup>
:::    <ClCompile Include="$(ProjectDir)..\..\dtvoicemgr.c" />
:::  </ItemGroup>
:::  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
:::</Project>
:::'@
:::
:::Write-Src (Join-Path $mgrDir 'DtVoiceMgr.vcxproj') $mgrProj
