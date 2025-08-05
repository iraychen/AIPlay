@echo off
REM Build script for Qt Plugin Framework on Windows
REM This script assumes Qt is installed and qmake is in the PATH

echo Building Qt Plugin Framework...

REM Create build directories
mkdir build\debug 2>nul
mkdir build\release 2>nul
mkdir build\debug\plugins 2>nul
mkdir build\release\plugins 2>nul
mkdir build\debug\metadata 2>nul
mkdir build\release\metadata 2>nul
mkdir build\debug\config 2>nul
mkdir build\release\config 2>nul
mkdir build\debug\logs 2>nul
mkdir build\release\logs 2>nul

REM Set build mode (debug or release)
set BUILD_MODE=release
if "%1"=="debug" set BUILD_MODE=debug

echo Building in %BUILD_MODE% mode...

REM Build PluginCore first
cd PluginCore
qmake CONFIG+=%BUILD_MODE%
nmake %BUILD_MODE%
if %ERRORLEVEL% neq 0 (
    echo Error building PluginCore
    exit /b %ERRORLEVEL%
)
cd ..

REM Build HostApplication
cd HostApplication
qmake CONFIG+=%BUILD_MODE%
nmake %BUILD_MODE%
if %ERRORLEVEL% neq 0 (
    echo Error building HostApplication
    exit /b %ERRORLEVEL%
)
cd ..

REM Build Plugins
cd Plugins\MySqlBackup
qmake CONFIG+=%BUILD_MODE%
nmake %BUILD_MODE%
if %ERRORLEVEL% neq 0 (
    echo Error building MySqlBackup plugin
    exit /b %ERRORLEVEL%
)
cd ..\..

cd Plugins\SqlServerBackup
qmake CONFIG+=%BUILD_MODE%
nmake %BUILD_MODE%
if %ERRORLEVEL% neq 0 (
    echo Error building SqlServerBackup plugin
    exit /b %ERRORLEVEL%
)
cd ..\..

echo Build completed successfully!
echo The application is available in build\%BUILD_MODE%\HostApplication.exe