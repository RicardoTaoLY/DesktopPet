@echo off
setlocal enabledelayedexpansion

:: Get the directory where the script is located (project root)
set "project_root=%~dp0"

:: Detect build configuration (Debug/Release)
set "build_config=Release"
if exist "%project_root%bin\Debug" set "build_config=Debug"

:: Set target directory
set "target_dir=%project_root%bin\%build_config%"

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
    goto :error
)

:: Copy jsoncpp library files
set "lib_dir=%project_root%DesktopPet\lib"
if exist "%lib_dir%" (
    echo Copying library files...
    
    :: Copy jsoncpp.dll
    if exist "%lib_dir%\jsoncpp.dll" (
        copy /Y "%lib_dir%\jsoncpp.dll" "%target_dir%" >nul
    ) else (
        echo Warning: jsoncpp.dll not found
    )
    
    :: Copy jsoncpp.lib
    if exist "%lib_dir%\jsoncpp.lib" (
        copy /Y "%lib_dir%\jsoncpp.lib" "%target_dir%" >nul
    ) else (
        echo Warning: jsoncpp.lib not found
    )
) else (
    echo Error: DesktopPet\lib directory not found
    goto :error
)

:: Copy jsoncpp.exp if exists
if exist "%lib_dir%\jsoncpp.exp" (
    copy /Y "%lib_dir%\jsoncpp.exp" "%target_dir%" >nul 2>&1
)

:: Copy application icon if exists
if exist "%project_root%DesktopPet\main_icon.ico" (
    copy /Y "%project_root%DesktopPet\main_icon.ico" "%target_dir%" >nul
)

echo.
echo =====================================================
echo Resources copied successfully to: %target_dir%
echo Frames: %target_dir%\frames
echo Libraries: %target_dir%\*.dll, %target_dir%\*.lib
echo =====================================================
echo.
timeout /t 3 >nul
goto :eof

:error
echo.
echo !!! Error during copying !!!
echo Please check the project structure and try again.
echo.
timeout /t 5 >nul
exit /b 1