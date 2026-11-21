@echo off
cd /d "%~dp0"
echo ============================================
echo   ScanIT - Point of Sale GUI Build
echo ============================================
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if %errorlevel% neq 0 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
)
if %errorlevel% neq 0 (
    echo [ERROR] Visual Studio environment not found.
    pause
    exit /b 1
)

rc /nologo /fo resource.res resource.rc

cl /EHsc /O2 /std:c++17 /D_HAS_STD_BYTE=0 /DNOMINMAX /DWIN32_LEAN_AND_MEAN ^
  /I include ^
  /I ..\backend\include ^
  /I ..\SFML-2.5.1\include ^
  src\main.cpp ^
  src\PosApp.cpp ^
  src\UIUtils.cpp ^
  src\StaffManager.cpp ^
  src\ActivityLogger.cpp ^
  src\ProductCatalog.cpp ^
  src\InventoryManager.cpp ^
  src\CartService.cpp ^
  src\PosNavigation.cpp ^
  src\PosProductsView.cpp ^
  src\PosReceiptView.cpp ^
  src\PosReportsView.cpp ^
  src\PosModalsView.cpp ^
  src\SplashScreen.cpp ^
  src\LoginScreen.cpp ^
  ..\backend\src\Generator.cpp ^
  ..\backend\src\Decoder.cpp ^
  ..\backend\src\Validator.cpp ^
  ..\backend\src\CameraCapture.cpp ^
  ..\backend\src\ProductManager.cpp ^
  resource.res ^
  /link ^
  /LIBPATH:..\SFML-2.5.1\lib ^
  sfml-main.lib sfml-graphics.lib sfml-window.lib sfml-system.lib ^
  mfplat.lib mf.lib mfreadwrite.lib mfuuid.lib ole32.lib ^
  comdlg32.lib user32.lib gdi32.lib opengl32.lib ^
  /out:..\barcode_gui.exe

if %errorlevel% equ 0 (
    del *.obj resource.res >nul 2>&1
    copy /y ..\SFML-2.5.1\bin\*.dll .. >nul
    if not exist "..\assets" mkdir "..\assets"
    xcopy /y /e /i assets\* ..\assets >nul
    echo.
    echo [SUCCESS] barcode_gui.exe built successfully in project root with app icon!
) else (
    echo.
    echo [ERROR] Build failed. Check errors above.
)
