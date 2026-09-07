@echo off
setlocal

set "BUILD_DIR=%~2"
if not defined BUILD_DIR set "BUILD_DIR=build"
set "TEST_CONFIG="
if /I "%~1"=="test" set "TEST_CONFIG=-DBUILD_TESTING=ON"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo Visual Studio Build Tools 2022 was not found.
  exit /b 1
)

for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
if not defined VSINSTALL (
  echo The Visual Studio C++ x64 toolchain was not found.
  exit /b 1
)

call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1

set "CMAKE=%VSINSTALL%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "NINJA=%VSINSTALL%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
pushd "%~dp0"
"%CMAKE%" -S . -B "%BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=Release %TEST_CONFIG% "-DCMAKE_MAKE_PROGRAM=%NINJA%"
if errorlevel 1 goto test_failed

"%CMAKE%" --build "%BUILD_DIR%"
set "BUILD_RESULT=%errorlevel%"
if not "%BUILD_RESULT%"=="0" goto finish
if /I "%~1"=="test" (
  "%VSINSTALL%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir "%BUILD_DIR%" --output-on-failure
  if errorlevel 1 goto test_failed
)
:finish
popd
exit /b %BUILD_RESULT%

:test_failed
popd
exit /b 1
