# ScanIT — Smart Barcode POS & Inventory Studio

A complete, high-performance Code 39 barcode generator, optical reader, and modern Point-of-Sale (POS) inventory system developed for the **Introduction to Software Engineering (ISE)** course at FAST-NUCES.

**Project Team:** Built by Intruders  
**Language & Standard:** C++17  
**Build System:** Visual Studio 2022/2026 (`ScanIT.sln`) / MSVC / Portable Batch Scripts  
**GUI Framework:** SFML 2.5.1  
**Vision & Video Pipeline:** Windows Media Foundation API (`IMFSourceReader`)  

---

## Project Overview

ScanIT is a dual-interface desktop application featuring:
1. **Modern Dark Obsidian POS GUI (`barcode_gui.exe`):** A glassmorphic SFML desktop application providing full cashier terminal operations, real-time live webcam barcode scanning, cart checkout, stock replenishment, staff access control, and sales auditing.
2. **Interactive CLI Engine (`barcode_console.exe`):** A lightweight, keyboard-driven console application offering standalone generation, decoding, QA test suites, and database inspections without graphics overhead.

The core barcode pipeline is written entirely from scratch in standard C++ without third-party computer vision or image encoding dependencies.

---

## Visual Previews & System Diagrams

### Core Architecture & ERD
![Data Entity Relationship Diagram](docs/diagrams/Data%20%20Entity%20Relationship%20Diagram.png)

### Key Workflows
| End-to-End Happy Path | Optical Decode Pipeline |
| :---: | :---: |
| ![End-to-End Happy Path](docs/diagrams/End-to-End%20Happy%20Path%20Generate%20%20Decode.png) | ![Decode Barcode from BMP Flow](docs/diagrams/Decode%20Barcode%20from%20BMP%20Flow.png) |

| Camera Scan Stream | Test Suite Flow |
| :---: | :---: |
| ![Camera Scan Flow](docs/diagrams/Camera%20Scan%20Flow%20(Part%203%20%E2%80%94%20Increment%201%20stub).png) | ![Test Suite Flow](docs/diagrams/Test%20Suite%20Flow.png) |

---

## Core Capabilities & Features

### 1. Point of Sale (POS) Terminal & Smart Cart
- **Real-Time Live Camera Scanner:** Continuous hardware-accelerated webcam capture at 30 FPS via Windows Media Foundation without blocking the 60 FPS UI rendering thread.
- **Adaptive Screen & Glare Binarization:** Local moving-average thresholding and sentinel pattern sliding-window search (`NWNNWNWNN`) reads barcodes reliably off mobile phone screens, crinkled paper, and glares.
- **Dynamic Cart Management:** One-click barcode additions, manual code entry, quantity increments, price calculations, and subtotal computation.
- **Flexible Payment Strategies (Design Pattern):** Strategy Pattern implementation (`PaymentStrategy.h`) supporting both Cash and Card payment workflows with change calculation and card verification.
- **Receipt Generation:** Produces clean ASCII print receipts saved to `receipt.txt` with interactive on-screen modal review.

### 2. Barcode Generation & Verification
- **Code 39 Symbology:** Standard 1:3 narrow-to-wide bar ratios with `*` start/stop delimiters.
- **Direct 24-bit BMP Encoding:** Generates uncompressed 24-bit bitmaps directly into fixed stack buffers with zero dynamic heap allocation.
- **Strict Input Validation:** Enforces 6-digit numerical rules, rejects leading zeros (`012345`), empty inputs, and non-numeric symbols.

### 3. Inventory Management & Stock Auditing
- **Real-Time Inventory Tracking:** Stock counts auto-decrement upon sale completion and auto-increment upon refund or restock.
- **Stock Audit & Supplier Records:** Comprehensive procurement logging with supplier purchase history, unit cost, and restock timestamps.
- **Dual Persistent Mirroring:** Automatic synchronization between root and frontend data stores (`inventory.txt`, `product.txt`).

### 4. Order Returns & Refunds
- **Automated Return Flow:** Queries existing records in `Order.txt` and `Bill.txt`.
- **Restoration to Stock:** Automatically restores returned product quantity back to inventory shelves.
- **Audit Trail:** Logs refunds and customer returns directly to `activity_log.txt`.

### 5. Staff Access Control & Role Management
- **Role-Based Permissions:** Admin and Cashier authorization levels stored in `staff.txt`.
- **Security & Password Management:** Password reset flows and credentials verification.
- **Operator Logging:** Tracks all login, logout, and credential updates with operator timestamps.

### 6. Automated TDD Quality Assurance
- Automated test runner testing 9 unit and integration scenarios:
  1. Valid 6-digit input acceptance (`123456`)
  2. Leading-zero rejection (`012345`)
  3. Underlength rejection (< 6 digits)
  4. Overlength rejection (> 6 digits)
  5. Alphanumeric rejection (`12A456`)
  6. Empty input handling
  7. End-to-end barcode bitmap generation
  8. Round-trip file decoding
  9. Ghost/missing file resilience

---

## Repository Structure

```
D:\3semester\ISE\ISE PROJECT\
├── ScanIT.sln               # Master Visual Studio solution (Frontend + Backend)
├── barcode_gui.exe          # Compiled standalone POS Desktop GUI binary
├── barcode_console.exe      # Compiled standalone Backend Console binary
├── sync_barcodes.bat        # Utility script to sync barcode assets
│
├── backend/                 # Core engine & CLI console application
│   ├── include/             # Header files (Camera, Decoder, Generator, TestSuite, etc.)
│   ├── src/                 # Implementation files & CLI main
│   ├── build.bat            # Backend MSVC build script
│   └── ScanIT_Backend.vcxproj
│
├── frontend/                # SFML Desktop GUI application
│   ├── assets/              # UI textures, fonts, icons, sound/visual assets
│   ├── include/             # POS models, controllers, and view headers
│   ├── src/                 # POS GUI implementation files & main entry point
│   ├── build_gui.bat        # Frontend MSVC build script
│   └── ScanIT_Frontend.vcxproj
│
├── assets/                  # Root runtime asset package for SFML application
├── barcodes/                # Generated Code 39 BMP barcode bitmaps
│
├── docs/                    # Project Documentation & Deliverables
│   ├── diagrams/            # Architecture, ERD, and workflow diagrams (.png, .svg)
│   ├── screenshots/         # POS & GUI feature screenshots
│   └── media/               # Presentation and demo video deliverables
│
├── SFML-2.5.1/              # SFML 2.5.1 C++ library (headers & link libraries)
├── bin/ & obj/              # Visual Studio build output directories
├── *.dll                    # SFML runtime DLL dependencies
└── *.txt                    # Active database ledgers (product, inventory, order, bill, etc.)
```

---

## Database & Ledger Schemas

The application uses flat-file storage for portability, human auditability, and immediate persistence:

| Database File | Purpose & Contents |
| :--- | :--- |
| `product.txt` | Product catalog with ID, barcode, name, unit type, price, cost, and stock count. |
| `inventory.txt` | Extended inventory audit details, category classifications, and supplier purchase history. |
| `Order.txt` | Record of transactions (Order ID, timestamp, employee ID, total, payment status). |
| `OrderDetail.txt` | Itemized line-items for every order (Order ID, product barcode, quantity, unit price, total). |
| `Bill.txt` | Billing summary (Bill ID, Order ID, gross total, discount, tax, net total, payment method). |
| `staff.txt` | User credentials, roles (`admin` / `cashier`), and employee identification. |
| `activity_log.txt` | Comprehensive chronological audit trail of logins, sales, restocks, and returns. |
| `receipt.txt` | Output text file of the most recently finalized customer transaction receipt. |

---

## Software Engineering Practices Applied

- **Separation of Concerns (SoC):** Strict separation between domain algorithms (`Generator`, `Decoder`), hardware adapters (`CameraCapture`), data storage managers (`ProductManager`, `InventoryManager`, `StaffManager`), and UI presentation layers (`PosProductsView`, `PosReceiptView`, `PosModalsView`).
- **Low Coupling & High Cohesion:** Backend logic operates independently of SFML and UI structures, enabling headless testing and CLI execution.
- **Design Patterns:** Strategy Pattern for interchangeable payment workflows (`PaymentStrategy`), Manager patterns for catalog and inventory operations.
- **Defensive Programming & Fail-Safe Defaults:** Multi-level file path fallbacks (`./`, `frontend/`, `assets/`) ensure resilient asset loading across different execution environments.
- **TDD (Test-Driven Development):** Barcode invariant validation and generation/decoding round-trips are thoroughly covered with regression test suites.

---

## How to Build & Run

### Prerequisites
- **OS:** Windows 10 or Windows 11 (64-bit)
- **Compiler:** Microsoft Visual C++ (MSVC) with C++17 support (Visual Studio 2022 Community / Professional)
- **SDKs:** Windows Media Foundation libraries (`mfplat.lib`, `mfreadwrite.lib`, `mfuuid.lib`, `ole32.lib`) — bundled with Windows SDK
- **Dependencies:** SFML 2.5.1 (bundled in repository under `SFML-2.5.1/`)

### Method 1: Visual Studio (Recommended)
1. Open `ScanIT.sln` in Visual Studio 2022.
2. Select `Release` or `Debug` configuration and `x64` platform.
3. Set `ScanIT_Frontend` as Startup Project (for GUI) or `ScanIT_Backend` (for CLI).
4. Press `Ctrl + F5` to compile and run.

### Method 2: Standalone Batch Scripts
- **Console Application:** Run `backend\build.bat` (compiles `barcode_console.exe` to root).
- **GUI Application:** Run `frontend\build_gui.bat` (compiles `barcode_gui.exe` to root with application icon).

### Method 3: Command Line (MSBuild)
```powershell
# Build both frontend and backend:
msbuild ScanIT.sln /p:Configuration=Release /p:Platform=x64

# Run the POS GUI:
.\bin\Release\barcode_gui.exe

# Run the Console Engine:
.\bin\Release\barcode_console.exe
```

---

## Running the Automated QA Test Suite

1. Launch `barcode_console.exe`.
2. Select option `[8] QA: Run Automated TDD Test Suite`.
3. The test harness executes all 9 test assertions and outputs a pass/fail matrix.
