@echo off
setlocal
cd /d "%~dp0"

echo ======================================================================
echo              APEX NATIONAL BANK - MANAGEMENT SYSTEM
echo ======================================================================

if not exist "build\bank_server.exe" (
    echo Binary build\bank_server.exe not found. Building now...
    call build.bat
    if %ERRORLEVEL% NEQ 0 (
        echo [ERROR] Build failed! Cannot run server.
        pause
        exit /b %ERRORLEVEL%
    )
)

echo Starting C++ Backend Server on port 8080...
echo Server running at: http://localhost:8080
echo.
echo Opening default web browser...
start http://localhost:8080
echo.
echo ======================================================================
echo Server is active. Press Ctrl+C in this console window to stop server.
echo ======================================================================

build\bank_server.exe 8080
pause
