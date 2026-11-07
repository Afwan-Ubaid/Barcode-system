# Smart Barcode System (ISE Sprint 1)

[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen)]()
[![Methodology](https://img.shields.io/badge/Methodology-Agile%20Scrum-blue)]()
[![Language](https://img.shields.io/badge/C%2B%2B-17-orange)]()

A modular, robust Barcode Generation and Recognition System developed for the **Introduction to Software Engineering (ISE)** course at FAST-NUCES.

---

## 🏛 Software Engineering Principles Applied
1. **Modularity & High Functional Cohesion (Lecture 07-08):**
   * `Validator.cpp`: Input sanitization and rule enforcement.
   * `Generator.cpp`: Encodes 6-digit number into standard Code 39 BMP format.
   * `Decoder.cpp`: Parses and decodes barcode images and camera frames.
   * `TestSuite.cpp`: Automated TDD verification suite.
2. **Low Coupling (Data Coupling):** Modules exchange only primitive types (`std::string`, `bool`).
3. **Anticipation of Change & Open-Closed Principle (Lecture 09):** Barcode engine is architected with clear interfaces, ready for 12-digit UPC or QR expansion in Sprint 2.
4. **Three Golden Rules of UI Design (Lecture 15):** Consistent prompts, user in control, low memory load.

---

## 👥 Team Work Breakdown Structure (WBS)
* **Member 1 (Domain Engineer):** `Generator.h`, `Generator.cpp` (Part 1 - `ISE-1`)
* **Member 2 (Computer Vision / Scanner):** `Decoder.h`, `Decoder.cpp` (Part 2 - `ISE-2` & `ISE-3`)
* **Member 3 (UI/UX Engineer):** `Validator.h`, `Validator.cpp`, `main.cpp` (`ISE-4`)
* **Member 4 (DevOps & QA Lead):** `TestSuite.h`, `TestSuite.cpp`, GitHub CI, Jira Management (`ISE-5`)

---

## 🚀 How to Build and Run

### Using the Build Script (Visual Studio MSVC):
Double-click or run from command prompt:
```cmd
build.bat
barcode_app.exe
```

### Manual Compilation:
```cmd
cl /EHsc /std:c++17 /I include src\Validator.cpp src\Generator.cpp src\Decoder.cpp src\TestSuite.cpp src\main.cpp /Fe:barcode_app.exe
```
