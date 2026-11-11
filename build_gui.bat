@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cl /EHsc /std:c++17 /I include /I SFML-2.5.1\include src\Validator.cpp src\Generator.cpp src\Decoder.cpp src\GUI.cpp src\main-sfml.cpp /link /LIBPATH:SFML-2.5.1\lib sfml-graphics.lib sfml-window.lib sfml-system.lib comdlg32.lib user32.lib gdi32.lib /out:barcode_gui.exe

if %errorlevel% equ 0 (
    copy /y SFML-2.5.1\bin\*.dll . >nul
    echo.
    echo [SUCCESS]: barcode_gui.exe built successfully!
)
