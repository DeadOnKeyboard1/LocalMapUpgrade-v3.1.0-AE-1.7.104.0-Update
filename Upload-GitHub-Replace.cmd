@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "REPO=DeadOnKeyboard1/LocalMapUpgrade-v3.1.0-AE-1.7.104.0-Update"
set "BRANCH=main"
set "ROOT=%~dp0"
set "SOURCE=%ROOT:~0,-1%"
set "WORK=%TEMP%\LMU-GitHub-Replace-%RANDOM%%RANDOM%"
set "CLONE=%WORK%\repo"
set "LOG=%ROOT%github-upload.log"

cd /d "%ROOT%" || goto :fatal

>"%LOG%" echo Local Map Upgrade GitHub replacement
>>"%LOG%" echo Repository: %REPO%
>>"%LOG%" echo Branch: %BRANCH%
>>"%LOG%" echo Source: %SOURCE%

echo.
echo ============================================================
echo  Local Map Upgrade - GitHub repository replacement
echo ============================================================
echo.
echo Repository: %REPO%
echo Branch    : %BRANCH%
echo.

call :ensure_git || goto :fail
call :ensure_gh || goto :fail

gh auth status -h github.com >>"%LOG%" 2>&1
if errorlevel 1 (
    echo [AUTH] GitHub login required.
    gh auth login -h github.com -p https -w
    if errorlevel 1 goto :fail
)

gh auth setup-git >>"%LOG%" 2>&1
if errorlevel 1 goto :fail

for /f "delims=" %%R in ('gh repo view "%REPO%" --json nameWithOwner --jq ".nameWithOwner" 2^>nul') do set "REMOTE_REPO=%%R"
if /I not "!REMOTE_REPO!"=="%REPO%" (
    echo [ERROR] Cannot access %REPO%.
    >>"%LOG%" echo ERROR: repository check failed: !REMOTE_REPO!
    goto :fail
)

if /I not "%~1"=="--yes" (
    echo This replaces the contents of GitHub main with this folder.
    echo A backup branch of the current main is created first.
    echo.
    set /p "CONFIRM=Type YES to continue: "
    if /I not "!CONFIRM!"=="YES" exit /b 0
)

if exist "%WORK%" rmdir /s /q "%WORK%" >nul 2>&1
mkdir "%WORK%" >nul 2>&1 || goto :fail

echo [1/6] Cloning current main...
git clone --branch "%BRANCH%" --single-branch "https://github.com/%REPO%.git" "%CLONE%" >>"%LOG%" 2>&1
if errorlevel 1 goto :fail

pushd "%CLONE%" || goto :fail
for /f "delims=" %%S in ('git rev-parse HEAD') do set "OLD_SHA=%%S"
for /f "delims=" %%T in ('git show -s --format^=%%cd --date^=format:%%Y%%m%%d-%%H%%M%%S HEAD') do set "STAMP=%%T"
if not defined OLD_SHA (
    popd
    goto :fail
)
if not defined STAMP set "STAMP=backup"
set "BACKUP=backup-before-replace-!STAMP!-%RANDOM%"

echo [2/6] Creating backup branch !BACKUP!...
git push origin "!OLD_SHA!:refs/heads/!BACKUP!" >>"%LOG%" 2>&1
if errorlevel 1 (
    popd
    goto :fail
)

echo [3/6] Removing old tracked repository files...
git rm -r -q . >>"%LOG%" 2>&1
if errorlevel 1 (
    popd
    goto :fail
)
popd

echo [4/6] Copying GitHub-ready source...
robocopy "%SOURCE%" "%CLONE%" /E /R:2 /W:1 /NFL /NDL /NJH /NJS /NP /XD .git _toolchain build build-logs dist package out .vs .vscode .idea .codex.tmp __pycache__ /XF *.dll *.pdb *.lib *.exp *.ilk *.map *.log *.zip *.rar *.7z *.tmp *.pyc >>"%LOG%" 2>&1
set "ROBOCOPY_RC=%ERRORLEVEL%"
if %ROBOCOPY_RC% GEQ 8 (
    >>"%LOG%" echo ERROR: robocopy returned %ROBOCOPY_RC%
    goto :fail
)

pushd "%CLONE%" || goto :fail

for /f "delims=" %%U in ('gh api user --jq ".login"') do set "GH_USER=%%U"
for /f "delims=" %%I in ('gh api user --jq ".id"') do set "GH_ID=%%I"
if defined GH_USER git config user.name "!GH_USER!"
if defined GH_USER if defined GH_ID git config user.email "!GH_ID!+!GH_USER!@users.noreply.github.com"

echo [5/6] Staging and committing replacement...
git add -A >>"%LOG%" 2>&1
if errorlevel 1 (
    popd
    goto :fail
)

git status --short
git status --short >>"%LOG%" 2>&1

git diff --cached --quiet
if not errorlevel 1 (
    echo [OK] Repository already matches this source. Nothing to upload.
    popd
    goto :success
)

git commit -m "Update Local Map Upgrade 1.7.104.0 - CTD and stability fixes" >>"%LOG%" 2>&1
if errorlevel 1 (
    popd
    goto :fail
)

echo [6/6] Pushing main...
git push origin "HEAD:%BRANCH%" >>"%LOG%" 2>&1
if errorlevel 1 (
    popd
    goto :fail
)

for /f "delims=" %%S in ('git rev-parse HEAD') do set "NEW_SHA=%%S"
popd

:success
echo.
echo ============================================================
echo  GITHUB UPDATE COMPLETE
echo ============================================================
echo Repository : https://github.com/%REPO%
if defined BACKUP echo Backup     : !BACKUP!
if defined NEW_SHA echo Commit     : !NEW_SHA!
echo Log        : "%LOG%"
echo.
if exist "%WORK%" rmdir /s /q "%WORK%" >nul 2>&1
exit /b 0

:ensure_git
where git >nul 2>&1 && exit /b 0
if exist "%ProgramFiles%\Git\cmd\git.exe" (
    set "PATH=%ProgramFiles%\Git\cmd;%PATH%"
    exit /b 0
)
where winget >nul 2>&1 || exit /b 1
echo [SETUP] Installing Git...
winget install --id Git.Git --exact --accept-package-agreements --accept-source-agreements >>"%LOG%" 2>&1
if exist "%ProgramFiles%\Git\cmd\git.exe" set "PATH=%ProgramFiles%\Git\cmd;%PATH%"
where git >nul 2>&1 || exit /b 1
exit /b 0

:ensure_gh
where gh >nul 2>&1 && exit /b 0
if exist "%ProgramFiles%\GitHub CLI\gh.exe" (
    set "PATH=%ProgramFiles%\GitHub CLI;%PATH%"
    exit /b 0
)
where winget >nul 2>&1 || exit /b 1
echo [SETUP] Installing GitHub CLI...
winget install --id GitHub.cli --exact --accept-package-agreements --accept-source-agreements >>"%LOG%" 2>&1
if exist "%ProgramFiles%\GitHub CLI\gh.exe" set "PATH=%ProgramFiles%\GitHub CLI;%PATH%"
where gh >nul 2>&1 || exit /b 1
exit /b 0

:fail
echo.
echo ============================================================
echo  GITHUB UPDATE FAILED
echo ============================================================
echo See log: "%LOG%"
echo Temporary clone kept at: "%WORK%"
echo.
exit /b 1

:fatal
echo [ERROR] Could not enter source directory.
exit /b 1
