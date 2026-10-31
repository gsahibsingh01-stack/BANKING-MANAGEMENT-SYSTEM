@echo off
echo ======================================================================
echo           Building Apex Banking Management System (C++ Backend)
echo ======================================================================

set VCVARS="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if exist %VCVARS% (
    call %VCVARS%
) else (
    echo Note: vcvars64.bat not found at default path, using current environment...
)

if not exist "..\build" mkdir "..\build"
if not exist "..\database" mkdir "..\database"

REM Compile sqlite3.obj if not exists
if not exist "..\build\sqlite3.obj" (
    echo Compiling SQLite 3 amalgamation...
    cl /c /O2 /Idependencies\sqlite /I..\dependencies\sqlite ..\dependencies\sqlite\sqlite3.c /Fo..\build\sqlite3.obj
)

echo Compiling C++ backend source files...
cl /std:c++17 /EHsc /O2 /W3 ^
   /I..\dependencies /I..\dependencies\sqlite /Iinclude /I..\backend\include ^
   ..\backend\src\main.cpp ..\backend\src\database.cpp ..\backend\src\auth.cpp ..\backend\src\controllers.cpp ^
   ..\build\sqlite3.obj ^
   /Fe..\build\bank_server.exe ^
   ws2_32.lib crypt32.lib advapi32.lib

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ======================================================================
    echo  [SUCCESS] Apex Bank Server build complete: build\bank_server.exe
    echo ======================================================================
) else (
    echo.
    echo [ERROR] Build failed! Check compiler errors above.
    exit /b %ERRORLEVEL%
)
