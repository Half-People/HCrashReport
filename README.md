<!-- Badges -->
<p align="center">
  <img src="https://img.shields.io/badge/License-MIT-blue.svg" alt="License">
  <img src="https://img.shields.io/badge/Platform-Windows-green.svg" alt="Platform">
  <img src="https://img.shields.io/badge/Language-C++-blue.svg" alt="Language">
</p>

# 📊 HCrashReport

A Windows C++ crash reporting library — capture crashes, auto-generate minidumps & detailed reports, and visualize with a built-in ImGui viewer.

## ⚙️ Overview

`HCrashReport` is a lightweight C++ library that provides:

- Automatic **unhandled exception capture** (SEH-based)
- Generates **MiniDump (.dmp)** and **JSON crash reports**
- Provides a **GUI Crash Viewer** (built on ImGui + SDL2)
- Easy integration — **single API call** to enable
- Works on **Windows (x64)** Visual Studio projects

---

## 📂 Repository Structure

```
HCrashReport/
├─ CrashReport/           # GUI Viewer (Dear ImGui + SDL2)
├─ api/                   # Third-party dependencies (ImGui, SDL2, stb_image, nlohmann/json)
├─ HERROR_TEST/           # Sample app demonstrating crash capture
├─ RuningTime/            # (Optional) runtime utilities
├─ x64/Release/           # Pre-built binaries (CrashReport.exe)
├─ HERROR_TEST.sln        # Visual Studio solution
└─ LICENSE                # MIT License
```

---

## 🛠️ Features (Detailed)

### 🐞 Unhandled Exception Capture
- Installs **Vectored Exception Handler** + **SEH filter**
- Uses `MiniDumpWriteDump` (DbgHelp) to save `.dmp`
- Collects:
  - Crash address + code
  - Call stack (backtrace)
  - Loaded module info (DLLs, EXEs)

### 📄 JSON Crash Report
- Uses **nlohmann/json** to serialize crash metadata
- Generates file: `CrashReport/Crash/CrashMessage.HCrash`
- JSON contains:
  - Exception type + message
  - Stack frames (function, file, line)
  - System info (OS version, CPU)
  - Loaded modules (address, size, path)

### 🎨 GUI Viewer
- `CrashReport.exe`
- Built using **Dear ImGui** + **SDL2** + **stb_image**
- Features:
  - Load `.HCrash` JSON + `.dmp` file
  - Display stack trace, module list
  - Image previews (e.g., application icon)

### 🔗 Easy Integration
- Single header API: `HCrashReport.h`
- **Only one call needed:**
  ```cpp
  HCrashReport::RegisterCrashReporter();
  ```
- Safe to call at any point in `main()` or initialization code

---

## 📚 Public API Reference

### `void HCrashReport::RegisterCrashReporter()`
> Installs crash handlers. Call once at app start.

- Sets vectored exception handler
- Prepares dump & report folder paths (`CrashReport/Crash`)
- Safe to call multiple times (idempotent)

### `void HCrashReport::UnRegisterCrashReporter()` *(optional)*
> Removes installed handlers (cleanup)

---

## 📦 Installation & Build

1. **Clone repository**:
   ```bash
   git clone https://github.com/Half-People/HCrashReport.git
   ```
2. **Open** `HERROR_TEST.sln` in **Visual Studio (2019/2022)**
3. **Build** (Debug/Release) — dependencies are bundled under `api/`

---

## 🚀 Usage Guide

### 1️⃣ Setup in your project
```cpp
#include "HCrashReport.h"
int main() {
   HCrashReport::RegisterCrashReporter();
   // your app code ...
}
```

### 2️⃣ Trigger crash (example)
```cpp
int* p = nullptr;
*p = 42; // Crash
```

### 3️⃣ On crash, auto-generated:
- `CrashReport/Crash/CrashMessage.HCrash` (JSON)
- `CrashReport/Crash/*.dmp` (MiniDump)

### 4️⃣ View crash report
```
CrashReport/CrashReport.exe
```
Load `.HCrash` to inspect details visually

---

## 👥 Sample Application

HERROR_TEST demonstrates how crashes are captured.

```
HERROR_TEST.exe
```
Causes a deliberate crash on launch.

---

## 📝 Contributing

1. Fork repository
2. Create feature branch
3. Submit Pull Request 🚀

---

## 📜 License

MIT License — see [`LICENSE`](LICENSE) for details.

