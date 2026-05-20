# Excel Export Setup Guide

## Overview

The Excel export feature now uses a Python helper script with `openpyxl` library. This approach is simpler, more maintainable, and cross-platform.

## Prerequisites

### 1. Install Python Dependencies

```bash
pip install openpyxl
```

Verify installation:
```bash
python3 -c "import openpyxl; print('openpyxl version:', openpyxl.__version__)"
```

### 2. Build Configuration

The C++ build no longer requires external Excel libraries. Just standard Qt6:

```bash
# In CLion:
Build → Clean Project
Build → Build Project
```

Or from command line:
```bash
cd /path/to/FinDashCpp
rm -rf cmake-build-debug  # Optional: clean if there are CMake cache issues
mkdir -p cmake-build-debug
cd cmake-build-debug
cmake ..
make -j4
```

## How It Works

### C++ Side (src/export_xlsx.cpp)
1. **Collects data**: Gathers dashboard and metrics data from LSK and Cloudbeds APIs
2. **Creates JSON**: Serializes the data into JSON format
3. **Locates Python script**: Searches for `excel_export.py` in:
   - App bundle (`Resources/excel_export.py`)
   - Development directory (`src/excel_export.py`)
   - Current working directory
4. **Calls Python**: Launches `python3 excel_export.py <json_data> <output_path>`
5. **Returns result**: Returns the filesystem path of the generated Excel file

### Python Side (src/excel_export.py)
1. **Accepts input**: Receives JSON data and output path as command-line arguments
2. **Creates workbook**: Uses `openpyxl` to create a new Excel workbook
3. **Formats data**: 
   - Currency formatting: `$#,##0.00`
   - Percentage formatting: `0.0%`
   - Headers with gray background
   - Proper column widths
4. **Writes formulas**: Excel formulas for variance calculations
5. **Saves file**: Writes to specified path

## Testing

### Step 1: Run in Mock Mode
```bash
./cmake-build-debug/FinDash --mock
```

### Step 2: Click Export Button
- The app should generate a daily report
- Look for the file in your config directory:
  - macOS: `~/.config/FinDash/DailyReport_YYYY-MM-DD.xlsx`
  - Linux: `~/.config/FinDash/DailyReport_YYYY-MM-DD.xlsx`
  - Windows: `%APPDATA%\FinDash\DailyReport_YYYY-MM-DD.xlsx`

### Step 3: Verify Excel File
- Open the file in Excel, Numbers, or LibreOffice
- Check:
  - ✓ Title "Financial Dashboard Report" appears
  - ✓ Date is displayed correctly
  - ✓ LSK revenue groups show data (Cafe Bar, Cafe Food, etc.)
  - ✓ Variance columns show calculations
  - ✓ Cloudbeds metrics appear (Room Revenue, ADR, RevPAR, etc.)
  - ✓ Currency formatting is correct
  - ✓ Formulas calculate correctly

## Troubleshooting

### Python Script Not Found
**Error**: `Excel export helper script not found`

**Solution**: 
1. Verify `src/excel_export.py` exists in your project
2. Check that the file permissions allow execution: `chmod +x src/excel_export.py`

### openpyxl Not Installed
**Error**: `ERROR: openpyxl not installed. Install with: pip install openpyxl`

**Solution**:
```bash
pip install openpyxl
```

### Python3 Not Found
**Error**: `python3: command not found`

**Solution**:
- Install Python 3: `brew install python3` (macOS)
- On Windows/Linux: Download from python.org or use package manager
- Verify: `python3 --version`

### Process Timeout
**Error**: `Excel export process timed out`

**Solution**:
- Usually indicates Python script is hung or very slow
- Check for Python syntax errors in `excel_export.py`
- Test the script manually: `python3 src/excel_export.py '{}' /tmp/test.xlsx`

### File Not Created
**Error**: `Output file was not created`

**Solution**:
1. Verify the config directory exists and is writable
2. Check disk space
3. Run manually: `python3 src/excel_export.py '{"date":"May 13, 2026"}' ~/test.xlsx`

## Customization

To modify the Excel export format:

1. Edit `src/excel_export.py`
2. Modify the `create_daily_report()` function
3. Add/remove columns, change formatting, adjust cell layout
4. No C++ recompilation needed—just restart the app

## File Locations

| Component | Location | Notes |
|-----------|----------|-------|
| C++ Wrapper | `src/export_xlsx.cpp` | Calls Python helper |
| Python Helper | `src/excel_export.py` | Does the actual Excel work |
| Header | `src/export_xlsx.h` | Interface definition |
| Output | Config directory | `DailyReport_YYYY-MM-DD.xlsx` |

## Cross-Platform Notes

### macOS
- Python 3 comes with Xcode command line tools or install via Homebrew
- Config directory: `~/.config/FinDash/`

### Linux
- Install Python: `sudo apt install python3 python3-pip`
- Install openpyxl: `pip3 install openpyxl`
- Config directory: `~/.config/FinDash/`

### Windows
- Download Python from python.org or Windows Store
- Install openpyxl: `pip install openpyxl` (in command prompt or PowerShell)
- Config directory: `%APPDATA%\FinDash\`

## Performance

- Typical export time: < 1 second
- Timeout: 10 seconds (configurable in export_xlsx.cpp)
- File size: ~5-10 KB per report
