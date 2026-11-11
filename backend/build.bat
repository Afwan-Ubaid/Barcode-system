@echo off
cd /d "%~dp0"
echo ============================================
echo   ScanIT - Backend Console Build
echo   Built by Intruders
echo ============================================
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Visual Studio environment not found.
    pause
    exit /b 1
)
cl /EHsc /std:c++17 /I include src\Validator.cpp src\Generator.cpp src\Decoder.cpp src\TestSuite.cpp src\CameraCapture.cpp src\main.cpp mfplat.lib mf.lib mfreadwrite.lib mfuuid.lib ole32.lib /Fe:..\barcode_console.exe
if %errorlevel% equ 0 (
    echo.
    echo [SUCCESS] barcode_console.exe built in project root!
) else (
    echo [ERROR] Build failed. Check errors above.
)
pause
