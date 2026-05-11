cmake_minimum_required(VERSION 3.24)
project(DupeCheck LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_SERVICE "Build Windows service binary" ON)

if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
    add_link_options(/SAFESEH:NO)
endif()

# xxhash from local implementation.
add_library(xxhash STATIC src/hashing/xxhash/xxhash.cpp)
target_include_directories(xxhash PUBLIC ${CMAKE_SOURCE_DIR})

file(GLOB SQLITE_FILES external/sqlite3/sqlite3.c external/sqlite3/sqlite3.h external/sqlite3/sqlite3ext.h)
add_library(sqlite3 STATIC ${SQLITE_FILES})
target_include_directories(sqlite3 PRIVATE ${CMAKE_SOURCE_DIR}/external/sqlite3)

file(GLOB IMGUI_SOURCES external/imgui/*.cpp external/imgui/backends/imgui_impl_win32.cpp external/imgui/imconfig.h)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${CMAKE_SOURCE_DIR}/external/imgui)

# Core library sources.
set(CORE_SOURCES
    src/hashing/HashEngine.cpp
    src/scanner/FileScanner.cpp
    src/scanner/CachedScannerService.cpp
    src/engine/DuplicateEngine.cpp
    src/database/DatabaseManager.cpp
    src/service/ServiceHost.cpp
    src/service/NamedPipeServer.cpp
    src/gui/ImGuiView.cpp
    src/gui/Controls.cpp
    src/gui/PreviewPanel.cpp
    src/gui/SettingsDialog.cpp
    src/utils/JsonConfig.cpp
)

add_library(dupecheck_core STATIC ${CORE_SOURCES})
target_link_libraries(dupecheck_core PRIVATE sqlite3 imgui xxhash bcrypt shlwapi shell32 advapi32)
target_include_directories(dupecheck_core PUBLIC ${CMAKE_SOURCE_DIR})

# GUI executable.
add_executable(dupecheck WIN32 src/main.cpp src/cli.cpp resources/dupecheck.rc)
target_link_libraries(dupecheck PRIVATE dupecheck_core)
set_target_properties(dupecheck PROPERTIES OUTPUT_NAME "DupeCheck")

# Windows service binary (console subsystem).
if(BUILD_SERVICE)
    add_executable(dupecheck_service src/service/ServiceHost.cpp src/cli.cpp)
    set_target_properties(dupecheck_service PROPERTIES WIN32_EXECUTABLE FALSE)
    target_link_libraries(dupecheck_service PRIVATE dupecheck_core msvcrt advapi32 kernel32 user32 gdi32)
    set_target_properties(dupecheck_service PROPERTIES OUTPUT_NAME "DupeCheckService")
endif()

if(EXISTS resources/appicon.ico)
    configure_file(resources/appicon.ico ${CMAKE_BINARY_DIR}/appicon.ico COPYONLY)
endif()

# Tests (standalone — no GoogleTest dependency).
if(BUILD_TESTS)
    enable_testing()
    file(GLOB TEST_SOURCES tests/*.cpp)
    add_executable(tests ${TEST_SOURCES})
    target_link_libraries(tests PRIVATE dupecheck_core)
    set_target_properties(tests PROPERTIES OUTPUT_NAME "DupeCheckTests")
endif()