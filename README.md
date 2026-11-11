# ScanIT — Smart Barcode Studio

A Code 39 barcode generator and optical reader developed for the **Introduction to Software Engineering (ISE)** course at FAST-NUCES (Sprint 1 / Increment 1).

**Project Team:** Built by Intruders  
**Language & Standard:** C++17  
**Build System:** Visual Studio 2022/2026 (`ScanIT.sln`) / MSVC / Batch Scripts  
**GUI Framework:** SFML 2.5.1  

---

## Project Overview

ScanIT is a dual-interface application (Modern Dark-Mode Desktop GUI + Interactive CLI Console) designed to handle generation and decoding of standard Code 39 barcodes for 6-digit identification numbers.

The backend is built from scratch without external image libraries for BMP handling or barcode encoding. For live camera scanning, it uses the native Windows Media Foundation API (`IMFSourceReader`) with asynchronous frame capture and adaptive image binarization, allowing it to scan barcodes directly off phone screens and paper.

---

## Core Features

### 1. Barcode Generation (Part 1)
- Validates 6-digit numerical codes (rejects non-numeric characters, inputs not equal to 6 digits, and numbers starting with `0`).
- Generates standard Code 39 symbology with `*` start/stop delimiters and standard narrow/wide bar ratios (1:3).
- Directly constructs 24-bit uncompressed `.bmp` image files in pure C++ using fixed stack memory buffers (no dynamic heap vector allocation).

### 2. Optical Barcode Reader & Camera Stream (Part 2)
- **Static File Decoding:** Parses any 24-bit uncompressed BMP file, inspects the raster byte matrix, and decodes the embedded 6-digit payload.
- **Continuous Live Webcam Stream:** Runs hardware-accelerated video capture at 30 FPS using Windows Media Foundation without blocking the 60 FPS UI thread.
- **Adaptive Screen & Glare Filtering:** Uses a local moving-average threshold and sentinel pattern sliding-window search (`NWNNWNWNN`) to read barcodes off mobile phone screens, even with screen reflections, backlight washout, or slight rotation (`-10°` to `+10°`).
- **Upside-Down / Reverse Scanning:** Supports bidirectional reads in both forward and reversed orientations.

### 3. User Interface (SFML Desktop Dashboard)
- Dark obsidian glassmorphic UI with animated splash screen, typewriter branding ("BUILT BY INTRUDERS"), live red laser sweep viewport, and real-time activity log timeline.
- Dynamic viewport scaling that fits any image resolution or camera aspect ratio without stretching or zoom distortion.
- Dual-build capability: Can be launched as a visual desktop app (`barcode_gui.exe`) or as a lightweight console tool (`barcode_console.exe`).

### 4. Automated TDD Test Suite (QA)
- Built-in test runner validating 9 unit and integration test cases covering:
  - Valid 6-digit inputs.
  - Leading zero rejection (`012345`).
  - Boundary lengths (< 6 and > 6 digits).
  - Alphanumeric / special character rejection.
  - Empty input handling.
  - End-to-end file generation.
  - End-to-end roundtrip decoding of generated images.
  - Missing/ghost file error recovery.

---

## Repository Structure

```
├── ScanIT.sln               # Master Visual Studio solution (Frontend + Backend)
├── barcode_gui.exe          # Compiled standalone GUI binary
├── barcode_console.exe      # Compiled standalone Console binary
│
├── backend/                 # Core engine & CLI application
│   ├── include/
│   │   ├── CameraCapture.h  # Windows Media Foundation async camera worker
│   │   ├── Decoder.h        # Optical decoding & adaptive binarization algorithms
│   │   ├── Generator.h      # Code 39 pattern encoder and BMP file writer
│   │   ├── TestSuite.h      # TDD unit test runner
│   │   └── Validator.h      # Input validation logic
│   ├── src/
│   │   ├── CameraCapture.cpp
│   │   ├── Decoder.cpp
│   │   ├── Generator.cpp
│   │   ├── TestSuite.cpp
│   │   ├── Validator.cpp
│   │   └── main.cpp         # Interactive CLI console entry point
│   ├── build.bat            # Backend standalone MSVC build script
│   └── ScanIT_Backend.vcxproj
│
├── frontend/                # SFML Desktop GUI Application
│   ├── assets/              # UI sprites, fonts, icons, sound/visual assets
│   ├── include/
│   │   ├── GenerateScreen.h # Generation view & live barcode preview
│   │   ├── HomeScreen.h     # Dashboard & activity history timeline
│   │   ├── ReadScreen.h     # Viewport, image upload & live camera scanning
│   │   ├── SplashScreen.h   # Animated loading & branding sequence
│   │   └── UITheme.h        # Design tokens, color palette & typography
│   ├── src/
│   │   ├── GenerateScreen.cpp
│   │   ├── HomeScreen.cpp
│   │   ├── ReadScreen.cpp
│   │   ├── SplashScreen.cpp
│   │   └── main.cpp         # GUI application entry point
│   ├── build_gui.bat        # Frontend standalone MSVC build script
│   └── ScanIT_Frontend.vcxproj
│
└── assets/                  # Root assets for runtime lookup
```

---

## Software Engineering Practices Applied

- **High Functional Cohesion:** Each module is strictly responsible for one domain (`Validator` sanitizes, `Generator` writes bitmaps, `Decoder` interprets scanlines, `CameraCapture` feeds frames).
- **Low Coupling:** Backend modules do not depend on SFML or the frontend; communication happens through primitive types and standard C++ types.
- **Robust Error Handling:** File I/O checks, memory bounds enforcement, and defensive image header validation prevent crashes on corrupt or missing files.
- **TDD (Test-Driven Development):** Validation rules and encoding/decoding invariants were verified through automated tests before assembling the graphical interface.

---

## Team Breakdown (Built by Intruders)

- **Domain Logic & File Generation (`Generator.h/.cpp`):** Code 39 bit-pattern mapping, 24-bit uncompressed BMP byte-level serializer, stack buffer optimization.
- **Computer Vision & Scanner (`Decoder.h/.cpp`, `CameraCapture.h/.cpp`):** Moving-average adaptive thresholding, multi-angle line sampler, Windows Media Foundation integration.
- **Frontend & UX Engineering (`HomeScreen`, `GenerateScreen`, `ReadScreen`, `SplashScreen`):** SFML interface, asset pipeline, viewport auto-fit, real-time activity callbacks.
- **Input Validation & QA (`Validator.h/.cpp`, `TestSuite.h/.cpp`):** Input rules enforcement, automated test runner, build system configuration.

---

## How to Build & Run

### Prerequisites
- Windows 10 or Windows 11 (64-bit)
- Visual Studio 2022 (with "Desktop development with C++" workload)
- Windows SDK (comes with Visual Studio, provides `mfplat.lib`, `mfreadwrite.lib`, `mfuuid.lib`)
- SFML 2.5.1 (Visual C++ 17 64-bit) placed in the project root or configured in project properties

### Method 1: Using Visual Studio (Recommended)
1. Open `ScanIT.sln` in Visual Studio.
2. Set configuration to `Debug` or `Release` and platform to `x64`.
3. Right-click the desired startup project:
   - `ScanIT_Frontend` for the graphical interface.
   - `ScanIT_Backend` for the console version.
4. Press `F5` or `Ctrl + F5` to build and run.

### Method 2: Command Line (MSBuild)
From the Developer Command Prompt or PowerShell:
```powershell
# Build both GUI and Console applications:
msbuild ScanIT.sln /p:Configuration=Debug /p:Platform=x64

# Run the GUI:
.\bin\Debug\barcode_gui.exe

# Run the Console:
.\bin\Debug\barcode_console.exe
```

### Method 3: Using Batch Scripts
- For console only: Run `backend\build.bat`
- For GUI: Run `frontend\build_gui.bat`

---

## Running the Automated Test Suite

To run all 9 TDD test cases:
1. Start `barcode_console.exe`.
2. Select option `[4] QA: Run Automated TDD Test Suite`.
3. The console will display a detailed breakdown of all test assertions and their pass/fail status.
