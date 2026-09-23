@echo off
setlocal EnableExtensions

rem ============================================================================
rem Local Map Upgrade - Skyrim AE 1.7.104.0 build + package
rem
rem Usage:
rem   Build-1.7.104.0.cmd
rem   Build-1.7.104.0.cmd clean
rem
rem AUTO_INSTALL=1 allows winget to install missing build tools where possible.
rem Set this to 0 if you only want dependency checks.
rem ============================================================================

set "AUTO_INSTALL=1"
set "ROOT=%~dp0"
cd /d "%ROOT%" || goto :fatal

set "BUILD_DIR=%ROOT%build\relwithdebinfo-se-only"
set "DIST_DIR=%ROOT%dist"
set "TOOLCHAIN_DIR=%ROOT%_toolchain"
set "LMU_VCPKG_ROOT=%TOOLCHAIN_DIR%\vcpkg"
set "VCPKG_ROOT=%LMU_VCPKG_ROOT%"
set "PACKAGE_NAME=LocalMapUpgrade-3.1.0-AE-1.7.104.0.zip"

for /f %%I in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd-HHmmss"') do set "STAMP=%%I"
if not defined STAMP set "STAMP=unknown"
set "LOG_DIR=%ROOT%build-logs"
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%" >nul 2>&1
set "LOG=%LOG_DIR%\LMU-build-%STAMP%.log"

>"%LOG%" echo Local Map Upgrade build log - %DATE% %TIME%
>>"%LOG%" echo Root: %ROOT%
>>"%LOG%" echo Target runtime: Skyrim 1.7.104.0

echo.
echo ============================================================
echo  Local Map Upgrade - Skyrim AE 1.7.104.0
echo ============================================================
echo.
echo Log: "%LOG%"
echo.

if /I "%~1"=="clean" (
    echo [CLEAN] Removing previous build/package output...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
    if exist "%ROOT%package" rmdir /s /q "%ROOT%package"
    if exist "%DIST_DIR%\%PACKAGE_NAME%" del /q "%DIST_DIR%\%PACKAGE_NAME%"
)

call :find_or_install_vs || goto :fail
call :find_or_install_git || goto :fail
call :find_or_install_cmake || goto :fail
call :find_or_install_ninja || goto :fail
call :find_or_install_node || goto :fail
call :prepare_vcpkg || goto :fail

set "VCPKG_ROOT=%VCPKG_ROOT%"
set "VCPKG_DISABLE_METRICS=1"

rem Make sure CMake can see tools discovered/installed above.
>>"%LOG%" echo PATH=%PATH%
>>"%LOG%" echo VCPKG_ROOT=%VCPKG_ROOT%

echo [1/4] Configuring CMake + vcpkg...
call :run cmake --preset build-relwithdebinfo-se-only "-DCMAKE_MAKE_PROGRAM:FILEPATH=%NINJA_EXE%" || goto :fail

echo [2/4] Building LocalMapUpgrade.dll + PDB...
call :run cmake --build --preset relwithdebinfo-se-only --parallel || goto :fail

if not exist "%BUILD_DIR%\LocalMapUpgrade.dll" (
    >>"%LOG%" echo ERROR: Expected DLL missing: %BUILD_DIR%\LocalMapUpgrade.dll
    echo [ERROR] Build finished without LocalMapUpgrade.dll.
    goto :fail
)

echo [3/4] Building validated MCM/mod package tree...
set "LMU_BUILD_DIR=%BUILD_DIR%"
call :run node "%ROOT%tools\build-package.cjs" || goto :fail

if not exist "%DIST_DIR%" mkdir "%DIST_DIR%" >nul 2>&1
if exist "%DIST_DIR%\%PACKAGE_NAME%" del /q "%DIST_DIR%\%PACKAGE_NAME%"

echo [4/4] Creating Vortex/Nexus ZIP...
call :run powershell -NoProfile -ExecutionPolicy Bypass -Command "Compress-Archive -Path '%ROOT%package\*' -DestinationPath '%DIST_DIR%\%PACKAGE_NAME%' -CompressionLevel Optimal -Force" || goto :fail

if not exist "%DIST_DIR%\%PACKAGE_NAME%" (
    >>"%LOG%" echo ERROR: Package ZIP was not created.
    goto :fail
)

for %%F in ("%DIST_DIR%\%PACKAGE_NAME%") do set "ZIP_SIZE=%%~zF"

echo.
echo ============================================================
echo  BUILD SUCCESSFUL
echo ============================================================
echo DLL : "%BUILD_DIR%\LocalMapUpgrade.dll"
if exist "%BUILD_DIR%\LocalMapUpgrade.pdb" echo PDB : "%BUILD_DIR%\LocalMapUpgrade.pdb"
echo ZIP : "%DIST_DIR%\%PACKAGE_NAME%"
echo Log : "%LOG%"
echo.
>>"%LOG%" echo SUCCESS: %DIST_DIR%\%PACKAGE_NAME% (%ZIP_SIZE% bytes)
exit /b 0

:find_or_install_vs
set "VCVARS="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        if exist "%%I\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%%I\VC\Auxiliary\Build\vcvars64.bat"
    )
)
if not defined VCVARS (
    for %%E in (BuildTools Community Professional Enterprise) do (
        if exist "%ProgramFiles%\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat"
    )
)
if not defined VCVARS if "%AUTO_INSTALL%"=="1" (
    where winget >nul 2>&1
    if not errorlevel 1 (
        echo [SETUP] Visual Studio 2022 C++ Build Tools missing - installing...
        >>"%LOG%" echo Installing Microsoft.VisualStudio.2022.BuildTools
        winget install --id Microsoft.VisualStudio.2022.BuildTools --exact --silent --accept-package-agreements --accept-source-agreements --override "--wait --passive --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended" >>"%LOG%" 2>&1
        for %%E in (BuildTools Community Professional Enterprise) do (
            if exist "%ProgramFiles%\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat"
        )
    )
)
if not defined VCVARS (
    echo [ERROR] Visual Studio 2022 C++ Build Tools not found.
    >>"%LOG%" echo ERROR: Visual Studio 2022 C++ Build Tools not found.
    exit /b 1
)
echo [OK] Visual Studio C++ toolchain
call "%VCVARS%" >>"%LOG%" 2>&1
if errorlevel 1 exit /b 1
exit /b 0

:find_or_install_git
where git >nul 2>&1 && (
    echo [OK] Git
    exit /b 0
)
if exist "%ProgramFiles%\Git\cmd\git.exe" (
    set "PATH=%ProgramFiles%\Git\cmd;%PATH%"
    echo [OK] Git
    exit /b 0
)
if "%AUTO_INSTALL%"=="1" (
    where winget >nul 2>&1 && (
        echo [SETUP] Installing Git...
        winget install --id Git.Git --exact --silent --accept-package-agreements --accept-source-agreements >>"%LOG%" 2>&1
        if exist "%ProgramFiles%\Git\cmd\git.exe" set "PATH=%ProgramFiles%\Git\cmd;%PATH%"
    )
)
where git >nul 2>&1 || (
    echo [ERROR] Git is missing.
    >>"%LOG%" echo ERROR: Git is missing.
    exit /b 1
)
echo [OK] Git
exit /b 0

:find_or_install_cmake
where cmake >nul 2>&1 && (
    echo [OK] CMake
    exit /b 0
)
for %%P in (
    "%ProgramFiles%\CMake\bin"
    "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
) do if exist "%%~P\cmake.exe" set "PATH=%%~P;%PATH%"
where cmake >nul 2>&1 && (
    echo [OK] CMake
    exit /b 0
)
if "%AUTO_INSTALL%"=="1" (
    where winget >nul 2>&1 && (
        echo [SETUP] Installing CMake...
        winget install --id Kitware.CMake --exact --silent --accept-package-agreements --accept-source-agreements >>"%LOG%" 2>&1
        if exist "%ProgramFiles%\CMake\bin\cmake.exe" set "PATH=%ProgramFiles%\CMake\bin;%PATH%"
    )
)
where cmake >nul 2>&1 || (
    echo [ERROR] CMake is missing.
    >>"%LOG%" echo ERROR: CMake is missing.
    exit /b 1
)
echo [OK] CMake
exit /b 0


:find_or_install_ninja
set "NINJA_EXE="
for /f "delims=" %%I in ('where ninja 2^>nul') do if not defined NINJA_EXE set "NINJA_EXE=%%I"
if defined NINJA_EXE goto :ninja_ready

rem vcvars may put the VS-bundled Ninja on PATH, but also probe current VS
rem installations directly (including Visual Studio 2026 / version 18).
if defined VSINSTALLDIR if exist "%VSINSTALLDIR%Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" (
    set "NINJA_EXE=%VSINSTALLDIR%Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
    goto :ninja_ready
)
for %%P in (
    "%ProgramFiles%\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
    "%ProgramFiles%\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
    "%ProgramFiles%\Microsoft Visual Studio\18\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
    "%ProgramFiles%\Microsoft Visual Studio\18\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
    "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
) do if not defined NINJA_EXE if exist "%%~P\ninja.exe" set "NINJA_EXE=%%~P\ninja.exe"
if defined NINJA_EXE goto :ninja_ready

if "%AUTO_INSTALL%"=="1" (
    where winget >nul 2>&1 && (
        echo [SETUP] Installing Ninja...
        winget install --id Ninja-build.Ninja --exact --silent --accept-package-agreements --accept-source-agreements >>"%LOG%" 2>&1
        if exist "%LOCALAPPDATA%\Microsoft\WinGet\Links\ninja.exe" set "NINJA_EXE=%LOCALAPPDATA%\Microsoft\WinGet\Links\ninja.exe"
    )
)
if defined NINJA_EXE goto :ninja_ready

echo [SETUP] Winget Ninja was unavailable - downloading the latest official ninja-win.zip...
if not exist "%TOOLCHAIN_DIR%\ninja" mkdir "%TOOLCHAIN_DIR%\ninja" >nul 2>&1
powershell -NoProfile -ExecutionPolicy Bypass -Command "$ErrorActionPreference='Stop'; $r=Invoke-RestMethod 'https://api.github.com/repos/ninja-build/ninja/releases/latest'; $a=$r.assets ^| Where-Object name -eq 'ninja-win.zip' ^| Select-Object -First 1; if(-not $a){throw 'ninja-win.zip asset not found'}; Invoke-WebRequest $a.browser_download_url -OutFile '%TOOLCHAIN_DIR%\ninja\ninja-win.zip'; Expand-Archive -Path '%TOOLCHAIN_DIR%\ninja\ninja-win.zip' -DestinationPath '%TOOLCHAIN_DIR%\ninja' -Force" >>"%LOG%" 2>&1
if exist "%TOOLCHAIN_DIR%\ninja\ninja.exe" set "NINJA_EXE=%TOOLCHAIN_DIR%\ninja\ninja.exe"

:ninja_ready
if not defined NINJA_EXE (
    echo [ERROR] Ninja is missing.
    >>"%LOG%" echo ERROR: Ninja is missing.
    exit /b 1
)
for %%D in ("%NINJA_EXE%") do set "PATH=%%~dpD;%PATH%"
if not exist "%NINJA_EXE%" (
    echo [ERROR] Ninja path is invalid: "%NINJA_EXE%"
    >>"%LOG%" echo ERROR: Ninja path is invalid: %NINJA_EXE%
    exit /b 1
)
echo [OK] Ninja: "%NINJA_EXE%"
>>"%LOG%" echo NINJA_EXE=%NINJA_EXE%
exit /b 0

:find_or_install_node
where node >nul 2>&1 && (
    echo [OK] Node.js
    exit /b 0
)
if exist "%ProgramFiles%\nodejs\node.exe" (
    set "PATH=%ProgramFiles%\nodejs;%PATH%"
    echo [OK] Node.js
    exit /b 0
)
if "%AUTO_INSTALL%"=="1" (
    where winget >nul 2>&1 && (
        echo [SETUP] Installing Node.js LTS...
        winget install --id OpenJS.NodeJS.LTS --exact --silent --accept-package-agreements --accept-source-agreements >>"%LOG%" 2>&1
        if exist "%ProgramFiles%\nodejs\node.exe" set "PATH=%ProgramFiles%\nodejs;%PATH%"
    )
)
where node >nul 2>&1 || (
    echo [ERROR] Node.js is missing.
    >>"%LOG%" echo ERROR: Node.js is missing.
    exit /b 1
)
echo [OK] Node.js
exit /b 0

:prepare_vcpkg
rem vcvars64.bat may overwrite VCPKG_ROOT with Visual Studio's bundled vcpkg.
rem LMU always uses its own isolated copy under _toolchain so the build is
rem reproducible and we never try to git-clone into Program Files.
set "VCPKG_ROOT=%LMU_VCPKG_ROOT%"
if not exist "%TOOLCHAIN_DIR%" mkdir "%TOOLCHAIN_DIR%" >nul 2>&1

rem A valid local checkout needs either its .git metadata or, at minimum, the
rem bootstrap script + vcpkg CMake toolchain. Remove only our own broken local
rem folder; never touch Visual Studio's vcpkg directory.
if exist "%VCPKG_ROOT%" (
    if not exist "%VCPKG_ROOT%\.git" goto :recreate_local_vcpkg
    if not exist "%VCPKG_ROOT%\bootstrap-vcpkg.bat" goto :recreate_local_vcpkg
    if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" goto :recreate_local_vcpkg
    goto :local_vcpkg_ready
)
goto :clone_local_vcpkg

:recreate_local_vcpkg
echo [SETUP] Incomplete local vcpkg detected - recreating it...
>>"%LOG%" echo Removing incomplete local vcpkg: %VCPKG_ROOT%
rmdir /s /q "%VCPKG_ROOT%" >>"%LOG%" 2>&1
if exist "%VCPKG_ROOT%" (
    echo [ERROR] Could not remove incomplete local vcpkg.
    >>"%LOG%" echo ERROR: Could not remove %VCPKG_ROOT%
    exit /b 1
)

:clone_local_vcpkg
echo [SETUP] Downloading isolated vcpkg to _toolchain\vcpkg...
rem Do not use a shallow clone: vcpkg.json pins a builtin-baseline commit and
rem vcpkg must be able to resolve that history locally.
call :run git clone https://github.com/microsoft/vcpkg.git "%VCPKG_ROOT%" || exit /b 1

:local_vcpkg_ready

if not exist "%VCPKG_ROOT%\bootstrap-vcpkg.bat" (
    echo [ERROR] Local vcpkg checkout is incomplete.
    >>"%LOG%" echo ERROR: bootstrap-vcpkg.bat missing in %VCPKG_ROOT%
    exit /b 1
)

if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo [SETUP] Bootstrapping local vcpkg...
    rem IMPORTANT: bootstrap-vcpkg.bat is another batch file. It must be
    rem invoked through CALL from our wrapper, otherwise cmd.exe replaces
    rem the current batch context and the :run return label is lost.
    call :run_batch "%VCPKG_ROOT%\bootstrap-vcpkg.bat" -disableMetrics || exit /b 1
)

if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo [ERROR] vcpkg.exe is missing after bootstrap.
    >>"%LOG%" echo ERROR: vcpkg.exe missing after bootstrap: %VCPKG_ROOT%
    exit /b 1
)
if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    echo [ERROR] vcpkg CMake toolchain is missing.
    >>"%LOG%" echo ERROR: scripts\buildsystems\vcpkg.cmake missing: %VCPKG_ROOT%
    exit /b 1
)

>>"%LOG%" echo Using isolated VCPKG_ROOT=%VCPKG_ROOT%
echo [OK] vcpkg: "%VCPKG_ROOT%"
exit /b 0

:run
>>"%LOG%" echo.
>>"%LOG%" echo ^> %*
%* >>"%LOG%" 2>&1
set "RC=%ERRORLEVEL%"
if not "%RC%"=="0" (
    >>"%LOG%" echo ERRORLEVEL=%RC%
    exit /b %RC%
)
exit /b 0

:run_batch
>>"%LOG%" echo.
>>"%LOG%" echo ^> CALL %*
call %* >>"%LOG%" 2>&1
set "RC=%ERRORLEVEL%"
if not "%RC%"=="0" (
    >>"%LOG%" echo ERRORLEVEL=%RC%
    exit /b %RC%
)
exit /b 0

:fail
echo.
echo ============================================================
echo  BUILD FAILED
echo ============================================================
echo See log:
echo "%LOG%"
echo.
exit /b 1

:fatal
echo Could not enter source directory: "%ROOT%"
exit /b 1
