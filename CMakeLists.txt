cmake_minimum_required(VERSION 3.24)
project(DupeCheck LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# MSVC defaults to /W1 and /Gy- in Debug — standardize.
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
endif()

add_library(xxhash STATIC src/hashing/xxhash/xxhash.cpp)
target_include_directories(xxhash PUBLIC ${CMAKE_SOURCE_DIR})

file(GLOB SQLITE_FILES external/sqlite3/sqlite3.c
                        external/sqlite3/sqlite3.h
                        external/sqlite3/sqlite3ext.h)
add_library(sqlite3 STATIC ${SQLITE_FILES})
target_include_directories(sqlite3 PRIVATE ${CMAKE_SOURCE_DIR}/external/sqlite3)

file(GLOB IMGUI_SOURCES
    external/imgui/*.cpp
    external/imgui/backends/imgui_impl_win32.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${CMAKE_SOURCE_DIR}/external/imgui)

# Core sources — explicitly listed to avoid duplicate symbols when linked.
set(CORE_SOURCES
    src/database/DatabaseManager.cpp
    src/engine/DuplicateEngine.cpp
    src/hashing/HashEngine.cpp
    src/scanner/CachedScannerService.cpp
    src/service/NamedPipeServer.cpp
)

add_library(dupecheck_core STATIC ${CORE_SOURCES})
target_link_libraries(dupecheck_core PRIVATE sqlite3 imgui xxhash bcrypt shlwapi shell32 advapi32)
target_include_directories(dupecheck_core PUBLIC ${CMAKE_SOURCE_DIR})

# GUI executable (no console window shown by default).
add_executable(dupecheck WIN32 src/main.cpp
                               src/gui/Controls.cpp
                               src/gui/ImGuiView.cpp
                               src/gui/PreviewPanel.cpp
                               src/gui/SettingsDialog.cpp)
target_link_libraries(dupecheck PRIVATE dupecheck_core)
set_target_properties(dupecheck PROPERTIES OUTPUT_NAME "DupeCheck")

# Windows service binary (console subsystem for CLI).
add_executable(dupeservice CONSOLE src/service/ServiceHost.cpp)
target_link_libraries(dupeservice PRIVATE dupecheck_core msvcrt advapi32 kernel32 user32 gdi32)
set_target_properties(dupeservice PROPERTIES OUTPUT_NAME "DupeCheckService")

if(EXISTS resources/appicon.ico)
    configure_file(resources/appicon.ico ${CMAKE_BINARY_DIR}/appicon.ico COPYONLY)
endif()