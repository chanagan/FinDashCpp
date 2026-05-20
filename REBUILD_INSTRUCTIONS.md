# Rebuild Instructions - Python-Based Excel Export

## Quick Start (5 minutes)

### 1. Install Python Dependencies
```bash
pip install openpyxl
```

### 2. Clean CLion Cache
In CLion:
- **File** → **Invalidate Caches** → **Clear All and Restart**

This clears old CMake configuration that referenced xlnt/QXlsx.

### 3. Rebuild
In CLion:
- **Build** → **Build Project**

Or command line:
```bash
cd /path/to/FinDashCpp/cmake-build-debug
cmake ..
make clean
make -j4
```

### 4. Test
```bash
./cmake-build-debug/FinDash --mock
```

Then click the "Export" button to generate an Excel file.

## What Changed

### Before (Old Approach)
```
xlnt C++ library → Couldn't find headers → Build errors
```

### Now (New Approach)
```
C++ collects data → Sends JSON to Python → openpyxl creates Excel → File saved
```

### Files Modified
1. **CMakeLists.txt** - Removed xlnt/QXlsx, simplified to just Qt6
2. **src/export_xlsx.cpp** - Complete rewrite to use QProcess + Python
3. **src/export_xlsx.h** - Updated documentation
4. **src/excel_export.py** - NEW: Python helper script

## Expected Behavior

### Build Output
```
[ 66%] Building CXX object CMakeFiles/FinDash.dir/src/export_xlsx.cpp.o
[100%] Linking CXX executable FinDash
[100%] Built target FinDash
```

No errors about xlsxwriter.h, xlnt, or QXlsx!

### Runtime Output
When exporting:
```
[Export] Saved → /Users/yourname/.config/FinDash/DailyReport_2026-05-13.xlsx
```

### File Verification
```bash
# Check that the file was created
ls -lh ~/.config/FinDash/DailyReport_*.xlsx

# Should see something like:
# -rw-r--r--  1 user  staff  8.5K May 13 10:30 DailyReport_2026-05-13.xlsx
```

## Troubleshooting

### "CMake Error: get_target_property() called with non-existent target"
This error means the CMake cache still has old configuration.

**Fix**: Use **File → Invalidate Caches → Clear All and Restart** in CLion

### "fatal error: 'xlsxwriter.h' file not found"
The build is using cached CMake configuration.

**Fix**: 
```bash
rm -rf cmake-build-debug/.cmake
cd cmake-build-debug
cmake ..
make clean
make -j4
```

### Build succeeds but export fails with "Python helper not found"
The script can't find `excel_export.py`.

**Fix**: Verify the file exists:
```bash
ls -la src/excel_export.py
```

If missing, the build might be incomplete. Check that all files were properly updated.

### "openpyxl not installed"
Error shown when trying to export.

**Fix**:
```bash
pip install openpyxl
python3 -c "import openpyxl; print('OK')"
```

### "python3: command not found"
Your system doesn't have Python 3.

**Fix**:
- **macOS**: `brew install python3`
- **Linux**: `sudo apt install python3`
- **Windows**: Download from python.org

Then verify: `python3 --version`

## Verification Steps

1. **Build compiles cleanly**
   ```bash
   cd cmake-build-debug
   make 2>&1 | grep -i error
   # Should show no errors
   ```

2. **Executable runs**
   ```bash
   ./FinDash --mock
   # Should start the app in mock mode
   ```

3. **Export works**
   - Click Export button in the app
   - Check output directory for DailyReport file
   - Open in Excel/Numbers to verify

4. **Excel file is valid**
   ```bash
   python3 -c "from openpyxl import load_workbook; wb = load_workbook('~/.config/FinDash/DailyReport_2026-05-13.xlsx'); print('Valid Excel file')"
   ```

## Next Steps

Once the build and export are working:

1. **Test with real API data** (if configured):
   ```bash
   ./FinDash  # Run with real API credentials
   # Generate real report
   ```

2. **Verify data accuracy**:
   - Compare exported Excel data with your hotel management systems
   - Check LSK revenue groups match
   - Verify Cloudbeds occupancy data

3. **Test on Windows** (if applicable):
   - Build on Windows using same instructions
   - Verify openpyxl works on Windows
   - Check file paths are correct for Windows config directory

## Support

If issues persist:

1. Check Python version: `python3 --version` (should be 3.7+)
2. Check openpyxl: `pip show openpyxl`
3. Test Python script directly:
   ```bash
   python3 src/excel_export.py '{"date":"Test"}' /tmp/test.xlsx
   ls -la /tmp/test.xlsx
   ```
4. Review CMakeLists.txt and verify no references to xlnt/QXlsx remain
5. Make sure CMAKE_AUTOMOC is still ON in CMakeLists.txt
