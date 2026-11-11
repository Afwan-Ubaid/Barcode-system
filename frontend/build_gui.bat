@echo off
cd /d "%~dp0"
echo ============================================
echo   ScanIT - SFML GUI Frontend Build
echo   Built by Intruders
echo ============================================
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Visual Studio environment not found.
    pause
    exit /b 1
)
cl /EHsc /std:c++17 /D_HAS_STD_BYTE=0 /DNOMINMAX /DWIN32_LEAN_AND_MEAN ^
  /I include ^
  /I ..\backend\include ^
  /I ..\SFML-2.5.1\include ^
  src\main.cpp ^
  src\SplashScreen.cpp ^
  src\HomeScreen.cpp ^
  src\GenerateScreen.cpp ^
  src\ReadScreen.cpp ^
  ..\backend\src\Generator.cpp ^
  ..\backend\src\Decoder.cpp ^
  ..\backend\src\Validator.cpp ^
  ..\backend\src\CameraCapture.cpp ^
  /link ^
  /LIBPATH:..\SFML-2.5.1\lib ^
  sfml-main.lib sfml-graphics.lib sfml-window.lib sfml-system.lib ^
  mfplat.lib mf.lib mfreadwrite.lib mfuuid.lib ole32.lib ^
  comdlg32.lib user32.lib gdi32.lib ^
  /out:..\barcode_gui.exe
if %errorlevel% equ 0 (
    copy /y ..\SFML-2.5.1\bin\*.dll .. >nul
    if not exist "..\assets" mkdir "..\assets"
    copy /y assets\*.* ..\assets\ >nul
    echo.
    echo [SUCCESS] barcode_gui.exe built in project root!
) else (
    echo [ERROR] Build failed. Check errors above.
)
pause
