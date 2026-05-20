# Excel Export Migration Summary

## Approach: Python Helper + C++ Bridge

Instead of struggling with C++ Excel libraries, we use a proven, cross-platform approach:
- **C++**: Collects financial data and creates JSON payload
- **Python**: Handles Excel generation using `openpyxl`
- **Communication**: Via `QProcess` with JSON/CLI arguments

This approach provides:
✓ Simplicity and maintainability  
✓ Cross-platform compatibility (Windows, macOS, Linux)  
✓ No complex C++ library bindings  
✓ Easy customization (edit Python script to change Excel format)  
✓ Proven libraries (openpyxl is widely used and stable)  

## Changes Made

### 1. CMakeLists.txt
- **Removed**: `FetchContent_Declare(xlnt ...)` and `FetchContent_Declare(QXlsx ...)`
- **Simplified**: No longer needs to link external Excel libraries
- **Requires**: Python 3.x with openpyxl installed

### 2. src/export_xlsx.h
- Updated documentation to reflect Python-based implementation
- No template file required
- Excel files created from scratch via Python helper

### 3. src/export_xlsx.cpp
- **Complete rewrite**: Uses `QProcess` to call Python helper
- **Data serialization**: Converts dashboard/metrics to JSON
- **Python script**: Located at `src/excel_export.py`
- Key features:
  - Finds Python script in multiple locations (app bundle, development, relative paths)
  - Passes JSON data to Python helper
  - Captures and reports errors
  - Returns filesystem path of generated Excel file

### 4. src/excel_export.py (new)
- Python helper script using `openpyxl`
- Handles all Excel generation logic
- Accepts JSON input from C++
- Supports:
  - Currency formatting ($#,##0.00)
  - Percentage formatting (0.0%)
  - Excel formulas (=C5-D5, variance calculations)
  - Conditional formatting via formulas
  - Proper column widths and fonts
  - Cloudbeds metrics optional data

## Output Format

The exported Excel file contains:
- **Title**: "Financial Dashboard Report" (14pt, bold)
- **Date**: Report date (bold)
- **Headers**: Revenue Category | Today | MTD | MTD-LY | Variance $ | Variance %
- **Data rows** (rows 4-9):
  - Cafe Bar
  - Cafe Food
  - Health Club
  - Laundry
  - Misc
  - Retail
- **Formulas**:
  - Variance $: `=C{row}-D{row}` (MTD - MTD-LY)
  - Variance %: `=IF(D{row}=0,0,(C{row}-D{row})/D{row})` (percentage change, handles division by zero)
- **Cloudbeds section** (if available):
  - Room Revenue (Today)
  - Occupied Rooms
  - Total Rooms
  - ADR (Today)
  - RevPAR (Today)
  - MTD Revenue

## Files Saved

Excel reports are saved to the **config directory** (typically `~/.config/FinDash/`) with naming:
```
DailyReport_YYYY-MM-DD.xlsx
```

## Testing Checklist

- [ ] Clean build succeeds (no compiler errors)
- [ ] Application starts without errors
- [ ] Mock mode runs without issues
- [ ] Export button generates Excel file
- [ ] Excel file opens in Excel/Numbers/LibreOffice
- [ ] Headers and data are visible and formatted correctly
- [ ] Variance formulas calculate correctly
- [ ] Cloudbeds data appears if configured
- [ ] File is created in correct location
- [ ] Cross-platform: Test on Windows (QXlsx supports both macOS and Windows)

## Benefits of QXlsx

1. **No external template required**: Creates files from scratch
2. **Cross-platform**: Works on macOS, Windows, and Linux
3. **No runtime dependencies**: Compiled into the application
4. **Better compatibility**: libxlsxwriter is mature and widely used
5. **Simpler code**: Direct API calls vs template manipulation
