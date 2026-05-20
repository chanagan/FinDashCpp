cmake_minimum_required(VERSION 3.20)
project(FinDash VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt MOC / RCC / UIC auto-generation
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ── Qt6 ───────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

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
set(QXLSX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/QXlsx/CMakeLists.txt")
set(QXLSX_SUBMODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/QXlsx/HelloWorld/QXlsx/CMakeLists.txt")

if(EXISTS "${QXLSX_PATH}")
    add_subdirectory(3rdparty/QXlsx)
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
