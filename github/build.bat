@echo off
setlocal EnableDelayedExpansion
title Magris Build - SpotifyLaunchers.exe + Setup
cd /d "%~dp0"

set "OK=0"
where g++ >nul 2>&1
if !errorlevel! equ 0 set "OK=1"

if "!OK!"=="0" (
  set "VCVARS="
  if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
  if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
  if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
  if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
  if defined VCVARS (
    echo [*] Visual Studio bulundu, ortam yukleniyor...
    call "!VCVARS!" >nul 2>&1
    where cl >nul 2>&1
    if !errorlevel! equ 0 set "OK=1"
  )
)

if "!OK!"=="0" (
  echo [HATA] g++ veya cl bulunamadi. MinGW veya Visual Studio kurun.
  pause
  exit /b 1
)

where g++ >nul 2>&1
if !errorlevel! equ 0 goto :mingw
goto :msvc

:mingw
echo [*] MinGW ile derleniyor...

echo [*] 1/2 - SpotifyLaunchers.exe (launcher)
if exist wezy.png if exist indir.jpg (
  windres resources.rc -O coff -o resources.o
  g++ SteelSeries.cpp resources.o -o SpotifyLaunchers.exe -static -std=c++17 -O2 -mwindows -DMAGRIS_LAUNCHER_EXE -lgdi32 -lgdiplus -lcomctl32 -lwininet -lole32 -lshlwapi -ladvapi32 -lshell32 -DUNICODE -D_UNICODE
  del resources.o 2>nul
) else (
  g++ SteelSeries.cpp -o SpotifyLaunchers.exe -static -std=c++17 -O2 -mwindows -DMAGRIS_LAUNCHER_EXE -lgdi32 -lgdiplus -lcomctl32 -lwininet -lole32 -lshlwapi -ladvapi32 -lshell32 -DUNICODE -D_UNICODE
)
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0pad_spotifyhelper.ps1" "%~dp0SpotifyLaunchers.exe"

echo [*] 2/2 - Setup (SpotifySetup.exe)
windres Setup.rc -O coff -o setup_res.o 2>nul
if exist setup_res.o (
  g++ FiveM.cpp SpotifyPePatch.cpp setup_res.o -o SpotifySetup.exe -static -std=c++17 -O2 -lgdi32 -lshell32 -lole32 -ladvapi32 -lwininet -mwindows -DUNICODE -D_UNICODE
  del setup_res.o 2>nul
) else (
  g++ FiveM.cpp SpotifyPePatch.cpp -o SpotifySetup.exe -static -std=c++17 -O2 -lgdi32 -lshell32 -lole32 -ladvapi32 -lwininet -mwindows -DUNICODE -D_UNICODE
)
goto :done

:msvc
echo [*] MSVC ile derleniyor...

echo [*] 1/2 - SpotifyLaunchers.exe
rc /nologo resources.rc 2>nul
cl /nologo SteelSeries.cpp resources.res /Fe:SpotifyLaunchers.exe /O2 /EHsc /DUNICODE /D_UNICODE /DMAGRIS_LAUNCHER_EXE gdi32.lib gdiplus.lib comctl32.lib wininet.lib ole32.lib user32.lib shlwapi.lib advapi32.lib shell32.lib /link /SUBSYSTEM:WINDOWS
del SteelSeries.obj resources.res 2>nul
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0pad_spotifyhelper.ps1" "%~dp0SpotifyLaunchers.exe"

echo [*] 2/2 - Setup (SpotifySetup.exe)
rc /nologo Setup.rc 2>nul
if exist Setup.res (
  cl /nologo FiveM.cpp SpotifyPePatch.cpp Setup.res /Fe:SpotifySetup.exe /O2 /EHsc /DUNICODE /D_UNICODE advapi32.lib shell32.lib user32.lib ole32.lib wer.lib wininet.lib /link /SUBSYSTEM:CONSOLE
) else (
  cl /nologo FiveM.cpp SpotifyPePatch.cpp /Fe:SpotifySetup.exe /O2 /EHsc /DUNICODE /D_UNICODE advapi32.lib shell32.lib user32.lib ole32.lib wer.lib wininet.lib /link /SUBSYSTEM:CONSOLE
)
del FiveM.obj SpotifyPePatch.obj SpotifySetup.obj Setup.res 2>nul
goto :done

:done
echo.
if exist SpotifySetup.exe echo [+] SpotifySetup.exe (Setup)
if exist SpotifyLaunchers.exe echo [+] SpotifyLaunchers.exe (launcher)
echo.
echo Kullanim:
echo   1. SpotifySetup (admin): SpotifyLaunchers.exe + kisayol yonlendirme
echo   2. Admin panel loader slotu: SpotifyLaunchers.exe yukleyin
pause
