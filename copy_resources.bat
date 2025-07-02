@echo off
setlocal enabledelayedexpansion

:: Get the directory where the script is located (project root)
set "project_root=%~dp0"

:: Define target directories
set "debug_dir=%project_root%bin\Debug"
set "release_dir=%project_root%bin\Release"

:: Check existence of directories
set "debug_exists=0"
set "release_exists=0"
if exist "%debug_dir%" set debug_exists=1
if exist "%release_dir%" set release_exists=1

:: Handle different directory scenarios
if %debug_exists% equ 0 if %release_exists% equ 0 (
    echo.
    echo !!! Error: no Debug/Release directory detected !!!
    echo Please build the project first to create Debug/Release folders.
    echo.
    goto :error
)

:: Set library directory
set "lib_dir=%project_root%DesktopPet\lib"

:: Copy to Debug if it exists
if %debug_exists% equ 1 (
    call :CopyResources "%debug_dir%"
    if errorlevel 1 goto :error
)

:: Copy to Release if it exists
if %release_exists% equ 1 (
    call :CopyResources "%release_dir%"
    if errorlevel 1 goto :error
)

echo.
echo =====================================================
echo Resources copied successfully to:
if %debug_exists% equ 1 echo   - Debug: %debug_dir%
if %release_exists% equ 1 echo   - Release: %release_dir%
echo =====================================================
echo.
timeout /t 3 >nul
goto :eof

:CopyResources
set "target_dir=%~1"
echo Copying to %target_dir%...

:: Create target directory if it doesn't exist
if not exist "%target_dir%" (
    echo Creating target directory: %target_dir%
    mkdir "%target_dir%"
)

:: Copy frames folder
if exist "%project_root%frames" (
    echo Copying frames folder...
    robocopy "%project_root%frames" "%target_dir%\frames" /E /XO /NFL /NDL /NJH /NJS >nul
) else (
    echo Error: frames folder not found
    exit /b 1
)

:: Copy jsoncpp library files
echo Copying library files...

:: Copy jsoncpp.dll
if not exist "%lib_dir%\jsoncpp.dll" (
    echo Warning: jsoncpp.dll not found
) else (
    copy /Y "%lib_dir%\jsoncpp.dll" "%target_dir%" >nul
)

:: Copy jsoncpp.lib
if not exist "%lib_dir%\jsoncpp.lib" (
    echo Warning: jsoncpp.lib not found
) else (
    copy /Y "%lib_dir%\jsoncpp.lib" "%target_dir%" >nul
)

:: Copy jsoncpp.exp if exists
if exist "%lib_dir%\jsoncpp.exp" (
    copy /Y "%lib_dir%\jsoncpp.exp" "%target_dir%" >nul 2>&1
)

:: Copy application icon if exists
if exist "%project_root%DesktopPet\art_toy.ico" (
    copy /Y "%project_root%DesktopPet\art_toy.ico" "%target_dir%" >nul
)

exit /b 0

:error
echo.
echo !!! Error during copying !!!
echo Please check the project structure and try again.
echo.
timeout /t 5 >nul
exit /b 1