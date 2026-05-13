cmake_minimum_required(VERSION 3.24)
project(DupeCheck LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
endif()

add_library(xxhash STATIC src/hashing/xxhash/xxhash.cpp)
target_include_directories(xxhash PUBLIC ${CMAKE_SOURCE_DIR})

# Explicitly list sqlite3 sources instead of using file(GLOB).
set(SQLITE_FILES
    external/sqlite3/sqlite3.c
    external/sqlite3/sqlite3.h
    external/sqlite3/sqlite3ext.h
)
add_library(sqlite3 STATIC ${SQLITE_FILES})
target_include_directories(sqlite3 PUBLIC ${CMAKE_SOURCE_DIR}/external/sqlite3)

# Explicitly list ImGui sources instead of using file(GLOB) for reproducibility.
set(IMGUI_SOURCES
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_win32.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${CMAKE_SOURCE_DIR}/external/imgui
                                        ${CMAKE_SOURCE_DIR}/external/imgui/backends)

set(CORE_SOURCES
    src/database/DatabaseManager.cpp
    src/hashing/HashEngine.cpp
    src/engine/DuplicateEngine.cpp
    src/scanner/CachedScannerService.cpp
    src/service/ServiceHost.cpp
    src/organization/OrganizationSvc.cpp
    src/utils/JsonConfig.cpp
    src/utils/DbPath.cpp
)

add_library(dupecheck_core STATIC ${CORE_SOURCES})
target_link_libraries(dupecheck_core PUBLIC sqlite3 imgui xxhash bcrypt shlwapi shell32 advapi32)
target_include_directories(dupecheck_core PUBLIC ${CMAKE_SOURCE_DIR})

add_executable(dupecheck src/main.cpp
                        src/gui/Controls.cpp
                        src/gui/ImGuiView.cpp
                        src/gui/PreviewPanel.cpp
                        src/gui/SettingsDialog.cpp)
target_link_libraries(dupecheck PRIVATE dupecheck_core opengl32.lib)
set_target_properties(dupecheck PROPERTIES OUTPUT_NAME "DupeCheck")
