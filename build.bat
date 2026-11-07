@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cl /EHsc /std:c++17 /I include src\Validator.cpp src\Generator.cpp src\Decoder.cpp src\TestSuite.cpp src\main.cpp /Fe:barcode_app.exe
