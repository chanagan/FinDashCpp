cmake_minimum_required(VERSION 3.20)
project(FinDash
        VERSION 1.0
        DESCRIPTION "FInancial Dashboard - CB + LSK"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt MOC / RCC / UIC auto-generation
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Set output directory for executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Qt path - platform specific
if(WIN32)
    set(CMAKE_PREFIX_PATH "C:/Qt/6.11.0/mingw_64")
elseif(APPLE)
    set(CMAKE_PREFIX_PATH "/Users/yourname/Qt/6.11.0/macos")
endif()

# ============================================================================
# Find Qt6 Components
# ============================================================================
# Required modules for this project:
# - Core: Basic Qt functionality (signals, slots, strings, files, etc.)
# - Gui: Low-level GUI (painting, fonts, icons, etc.)
# - Widgets: High-level GUI (windows, buttons, tables, etc.)
# - Network: HTTP networking, sockets, etc.
find_package(Qt6 REQUIRED COMPONENTS
        Core
        Gui
        Widgets
        Network)

# ── 3rdparty Libraries ───────────────────────────────────────────────────────
# Build 3rdparty libraries (QXlsx) from source
#add_subdirectory(3rdparty/QXlsx)



# ── Sources ───────────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/config.cpp
    src/models.cpp
    src/clients.cpp
    src/lsk_tokens.cpp
    src/lsk_api.cpp
    src/threads.cpp
    src/widgets.cpp
    src/dashboard.cpp
    src/export_xlsx.cpp
)

set(HEADERS
    src/config.h
    src/models.h
    src/clients.h
    src/lsk_tokens.h
    src/lsk_api.h
    src/threads.h
    src/widgets.h
    src/dashboard.h
    src/export_xlsx.h
)

add_executable(FinDash ${SOURCES} ${HEADERS})

target_link_libraries(FinDash PRIVATE
    Qt6::Widgets
    Qt6::Network
#    QXlsx
)

target_include_directories(FinDash PRIVATE src)

# ============================================================================
# QXlsx Library (for Excel Export)
# ============================================================================
# QXlsx is a pure Qt library for reading/writing Excel files

# Check both possible locations for QXlsx
set(QXLSX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/QXlsx/QXlsx/CMakeLists.txt")
set(QXLSX_SUBMODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/QXlsx/HelloWorld/QXlsx/CMakeLists.txt")
message(STATUS "path: ${QXLSX_PATH}")
message(STATUS "sub path: ${QXLSX_SUBMODULE_PATH}")

if(EXISTS "${QXLSX_PATH}")
    add_subdirectory(3rdparty/QXlsx/QXlsx)
    target_link_libraries(${PROJECT_NAME} PRIVATE QXlsx)
    target_compile_definitions(${PROJECT_NAME} PRIVATE QXLSX_LIBRARY)
    message(STATUS "QXlsx library enabled for Excel export (from 3rdparty/QXlsx)")
elseif(EXISTS "${QXLSX_SUBMODULE_PATH}")
    add_subdirectory(3rdparty/QXlsx/HelloWorld/QXlsx)
    target_link_libraries(${PROJECT_NAME} PRIVATE QXlsx)
    target_compile_definitions(${PROJECT_NAME} PRIVATE QXLSX_LIBRARY)
    message(STATUS "QXlsx library enabled for Excel export (from submodule)")
else()
    message(WARNING "QXlsx library not found. Excel export will be disabled.")
endif()

# ── macOS bundle (optional) ───────────────────────────────────────────────────
if(APPLE)
    set_target_properties(FinDash PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "FinDash"
    )
endif()
