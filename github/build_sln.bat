@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d "%~dp0"
msbuild setup.sln /p:Configuration=Release /p:Platform=x64 /v:minimal
