# Rebuild Instructions - QXlsx Fixed

## The Issue
CMake couldn't find QXlsx source files because FetchContent hadn't properly populated the directory before we tried to use it.

## The Fix
Updated CMakeLists.txt to:
1. Explicitly populate QXlsx using `FetchContent_Populate()`
2. Use `file(GLOB)` to dynamically find all .cpp files
3. Add debug messages showing where files are found

## How to Rebuild

### Step 1: Clean Build Cache
**This is critical!**

```bash
cd /path/to/FinDashCpp
rm -rf cmake-build-debug cmake-build-release
mkdir cmake-build-debug
cd cmake-build-debug
```

### Step 2: Run CMake
```bash
cmake ..
```

You should see output like:
```
[QXlsx] Fetching QXlsx...
[QXlsx] QXlsx source dir: /Users/chrishanagan/CLionProjects/FinDashCpp/cmake-build-debug/_deps/qxlsx-src
[QXlsx] Found /Users/chrishanagan/CLionProjects/FinDashCpp/cmake-build-debug/_deps/qxlsx-src/src
[QXlsx] Source files: <list of .cpp files>
```

If you see the message about source files found, you're good to go!

### Step 3: Build
```bash
make -j4
```

Or in CLion: **Build → Build Project**

## Alternative: CLion Full Clean

If you prefer to use CLion:

1. **File → Invalidate Caches → Clear All and Restart**
2. Wait for CLion to restart
3. **Build → Build Project**

CLion will automatically re-run CMake and rebuild from scratch.

## Expected Build Time

- **First time**: ~30-45 seconds (QXlsx + Qt + FinDash all compile)
- **Incremental**: ~5-10 seconds (only changed files)

## Expected Output

Should complete with:
```
[100%] Linking CXX executable FinDash
[100%] Built target FinDash
```

No errors!

## Test It

```bash
# Run in mock mode
./FinDash --mock

# In the app, click Export button
# Should create: ~/.config/FinDash/DailyReport_YYYY-MM-DD.xlsx

# Verify success
ls -lh ~/.config/FinDash/DailyReport_*.xlsx
```

## If You Get Build Errors

### Error: "xlsxwriter.h not found"
- Verify cmake output shows QXlsx source files found
- Check that cmake-build-debug/_deps/qxlsx-src/src/ exists
- Re-run: `rm -rf cmake-build-debug && mkdir cmake-build-debug && cd cmake-build-debug && cmake .. && make -j4`

### Error: "Cannot find source file"
- This means the paths are still being resolved incorrectly
- Verify qxlsx_SOURCE_DIR is being set
- Check cmake output for the path

### Build takes forever
- QXlsx has many source files - first compile is slow
- Ctrl+C to cancel and check for errors
- Check build output for actual errors vs just slow compilation

## Troubleshooting Tips

### See what CMake found
```bash
cd cmake-build-debug
cmake .. 2>&1 | grep -i WXlsx
```

### Check the build directory structure
```bash
ls -la _deps/WXlsx-src/src/*.cpp | head -10
# Should show 15-20 .cpp files
```

### Force a clean build
```bash
cd cmake-build-debug
cmake --build . --config Release --clean-first
```

## What's Different

This approach compiles QXlsx as part of FinDash, not as an external library:

```
OLD (broken):
  FinDash executable
  ↓ links against
  libQXlsx.a (external library)
  ↗ can't find headers

NEW (works):
  FinDash executable
  = compiled with
  xlsxwriter.cpp
  xlsxdocument.cpp
  ... (all QXlsx .cpp files)
  ✓ headers found automatically
```

## Success Indicators

✅ CMake output shows "QXlsx source files: /path/to/xlsxwriter.cpp ..."
✅ Build completes with "Built target FinDash"
✅ No header-not-found errors
✅ ./FinDash --mock runs
✅ Excel export creates file

Any of these mean you're on the right track!
