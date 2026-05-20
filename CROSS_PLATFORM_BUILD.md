# Cross-Platform Build Guide (macOS & Windows)

## Overview

FinDash now builds and runs on both macOS and Windows with the same source code. The key components:

- **C++**: Qt6 (cross-platform framework)
- **Python**: Helper script for Excel generation (included in bundle)
- **Dependencies**: Qt6, Python 3.x, openpyxl

## macOS Build & Deployment

### Prerequisites
```bash
# Install Xcode command line tools
xcode-select --install

# Install Qt6 (via Homebrew recommended)
brew install qt@6

# Install Python and openpyxl
brew install python3
pip3 install openpyxl
```

### Build
```bash
cd /path/to/FinDashCpp
mkdir -p cmake-build-release
cd cmake-build-release

cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=$(brew --prefix qt@6) ..
make -j$(sysctl -n hw.ncpu)
```

### Output
- **App bundle**: `FinDash.app` (ready to distribute)
- **Script location**: `FinDash.app/Contents/Resources/excel_export.py` (auto-deployed)
- **Config directory**: `~/.config/FinDash/`

### Distribution (macOS)
The app bundle is self-contained:
```bash
# Create dmg for distribution
hdiutil create -volname "FinDash" -srcfolder cmake-build-release/FinDash.app -ov -format UDZO FinDash.dmg

# Users can then:
# 1. Install Python 3 and openpyxl
# 2. Copy FinDash.app to Applications
# 3. Run normally
```

## Windows Build & Deployment

### Prerequisites

**Option A: MSVC (Visual Studio)**
```batch
REM Install Visual Studio 2022 Community (free)
REM Include: C++ development tools, Qt6 tools

REM Install Qt6
REM Download from qt.io or use vcpkg: vcpkg install qt6:x64-windows

REM Install Python 3
REM Download from python.org (add to PATH during install)
python -m pip install openpyxl
```

**Option B: MinGW**
```batch
REM Install Qt6 with MinGW
REM Download from qt.io

REM Install Python 3
REM Download from python.org

python -m pip install openpyxl
```

### Build (MSVC)
```batch
cd C:\path\to\FinDashCpp
mkdir cmake-build-release
cd cmake-build-release

cmake -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_PREFIX_PATH="C:\Qt\6.x\msvc2022_64" ..

cmake --build . --config Release
```

### Build (MinGW)
```batch
cd C:\path\to\FinDashCpp
mkdir cmake-build-release
cd cmake-build-release

cmake -G "MinGW Makefiles" ^
      -DCMAKE_PREFIX_PATH="C:\Qt\6.x\mingw_64" ..

cmake --build . --config Release -j 4
```

### Output
- **Executable**: `cmake-build-release\Release\FinDash.exe`
- **Script location**: `cmake-build-release\Release\excel_export.py` (auto-deployed)
- **Config directory**: `%APPDATA%\FinDash\`

### Distribution (Windows)

Create a deployment folder with all dependencies:
```batch
REM Create deployment directory
mkdir FinDash-Release
cd FinDash-Release

REM Copy executable
copy ..\cmake-build-release\Release\FinDash.exe .

REM Copy Python script
copy ..\src\excel_export.py .

REM Copy Qt6 DLLs (tool: windeployqt)
C:\Qt\6.x\msvc2022_64\bin\windeployqt.exe FinDash.exe

REM Now FinDash-Release contains all needed files
REM Users only need to install Python + openpyxl
```

Or use NSIS/WiX to create an installer.

## Cross-Platform Considerations

### 1. Python Executable
The code tries both `python3` and `python`:
- **macOS/Linux**: Typically `python3`
- **Windows**: Often just `python`
- Both are attempted automatically

### 2. File Paths
Using `std::filesystem` handles:
- Forward slashes (/) on macOS/Linux
- Backslashes (\) on Windows
- Both work interchangeably

### 3. Config Directory
Automatically set based on OS:
- **macOS**: `~/.config/FinDash/`
- **Linux**: `~/.config/FinDash/`
- **Windows**: `%APPDATA%\FinDash\`

See `src/config.cpp` for implementation.

### 4. Script Deployment

**CMake handles automatic deployment:**
```cmake
# All platforms: Copy to build directory
add_custom_command(...excel_export.py -> ${CMAKE_BINARY_DIR}/)

# macOS: Copy to app bundle
if(APPLE)
    add_custom_command(...excel_export.py -> FinDash.app/Contents/Resources/)
endif()

# Windows: Copy to exe directory
if(WIN32)
    add_custom_command(...excel_export.py -> $<TARGET_FILE_DIR:FinDash>/)
endif()
```

The C++ code searches multiple locations and logs where it finds the script.

## Testing Cross-Platform Build

### macOS Test
```bash
# Development
./cmake-build-release/FinDash.app/Contents/MacOS/FinDash --mock

# Or bundle
open cmake-build-release/FinDash.app
```

### Windows Test
```batch
REM Development
cmake-build-release\Release\FinDash.exe --mock

REM From command line
cd cmake-build-release\Release
FinDash.exe
```

### Both Platforms
1. Run in mock mode: `FinDash --mock`
2. Click Export button
3. Verify Excel file created:
   - macOS: `~/.config/FinDash/DailyReport_YYYY-MM-DD.xlsx`
   - Windows: `%APPDATA%\FinDash\DailyReport_YYYY-MM-DD.xlsx`
4. Open file in Excel/Numbers/LibreOffice
5. Verify data and formatting

## Troubleshooting Cross-Platform

### "python3: command not found" (Windows)
- Use `python` instead
- Or ensure Python is in PATH: `python --version`

### "DLL not found" (Windows)
- Run `windeployqt.exe FinDash.exe` in Release build directory
- This copies required Qt6 DLLs

### "Cannot find excel_export.py"
- Check build logs show copy succeeded:
  ```
  Copying excel_export.py to ...
  ```
- Verify file exists at expected location
- Check CMakeLists.txt has proper `add_custom_command`

### "openpyxl not found" (End users)
- Document in README that users must install:
  ```
  pip install openpyxl
  ```
- Or bundle Python interpreter (advanced)

## Deployment Checklist

### macOS
- [ ] Qt6 installed
- [ ] Python 3 + openpyxl installed
- [ ] Build succeeds: `make -j4`
- [ ] Executable runs: `./FinDash.app/Contents/MacOS/FinDash --mock`
- [ ] Excel export works
- [ ] Create DMG: `hdiutil create ...`
- [ ] README documents openpyxl requirement

### Windows
- [ ] Visual Studio or MinGW installed
- [ ] Qt6 installed and CMAKE_PREFIX_PATH set
- [ ] Python 3 + openpyxl installed
- [ ] Build succeeds: `cmake --build . --config Release`
- [ ] Executable runs: `FinDash.exe --mock`
- [ ] Excel export works
- [ ] Run `windeployqt.exe FinDash.exe`
- [ ] Test in clean deployment folder
- [ ] Create installer (NSIS/WiX) or zip
- [ ] README documents openpyxl requirement

## CI/CD Recommendations

For automated builds on both platforms:

### GitHub Actions Example
```yaml
name: Cross-Platform Build

on: [push, pull_request]

jobs:
  macos:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v3
      - run: brew install qt@6 python3
      - run: pip3 install openpyxl
      - run: mkdir build && cd build && cmake .. && make -j4
      - uses: actions/upload-artifact@v3
        with:
          name: FinDash-macOS
          path: build/FinDash.app

  windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      - run: pip install openpyxl
      - uses: jurplel/install-qt-action@v3
        with:
          version: '6.x'
      - run: mkdir build; cd build; cmake .. -G "Visual Studio 17 2022"
      - run: cmake --build . --config Release
      - uses: actions/upload-artifact@v3
        with:
          name: FinDash-Windows
          path: build/Release/FinDash.exe
```

This automates building for both platforms on every commit.
