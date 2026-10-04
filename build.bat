@echo off
setlocal EnableExtensions
cd /d "%~dp0"

set "ROOT=%~dp0"
set "OUT=%ROOT%build"
if not exist "%OUT%" mkdir "%OUT%"

set "CLANG="
if exist "%ROOT%tools\llvm-mingw\bin\clang.exe" set "CLANG=%ROOT%tools\llvm-mingw\bin\clang.exe"
if not defined CLANG (
  for /d %%D in ("%ROOT%tools\llvm-mingw-*") do (
    if exist "%%D\bin\clang.exe" set "CLANG=%%D\bin\clang.exe"
  )
)
if not defined CLANG (
  for /f "delims=" %%C in ('where clang 2^>nul') do (
    if not defined CLANG set "CLANG=%%C"
  )
)
if not defined CLANG (
  echo clang.exe was not found.
  echo Unpack LLVM MinGW into tools\llvm-mingw, or put clang on PATH.
  echo https://github.com/mstorsjo/llvm-mingw/releases
  exit /b 1
)

echo Using %CLANG%
"%CLANG%" --target=x86_64-w64-windows-gnu -shared -O2 -s -static -fno-stack-protector -Wall -Wextra -o "%OUT%\AutoChips.dll" "%ROOT%src\autochips.c" -lkernel32
if errorlevel 1 exit /b 1

"%CLANG%" --target=x86_64-w64-windows-gnu -O2 -s -static -fno-stack-protector -Wall -Wextra -Wno-unused-function -DAUTOCHIPS_FILE_CHECK -o "%OUT%\check_patterns.exe" "%ROOT%src\autochips.c" -lkernel32
if errorlevel 1 exit /b 1

echo Built %OUT%\AutoChips.dll
exit /b 0
