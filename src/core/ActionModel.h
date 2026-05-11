#pragma once
#include <string>
#include <vector>
#include "FileInfo.h"
#include <cstdint>

enum class FileType {
    Original,
    Duplicate
};

enum class ActionType {
    Rename,
    MoveToDuplicatesFolder,
    Delete,
    CreateSymlink,
    Archive,
};

struct ActionItem {
    FileInfo file;
    FileType type = FileType::Duplicate;
    std::string new_name;
    int copy_index = 0;
    ActionType action = ActionType::Rename;
    bool selected = false;
};

inline const char* action_type_to_string(ActionType t) {
    switch (t) {
        case ActionType::Rename:                  return "rename";
        case ActionType::MoveToDuplicatesFolder:  return "move";
        case ActionType::Delete:                  return "delete";
        case ActionType::CreateSymlink:           return "symlink";
        case ActionType::Archive:                 return "archive";
    }
    return "unknown";
}

inline const char* file_type_to_string(FileType t) {
    switch (t) {
        case FileType::Original:  return "original";
        case FileType::Duplicate: return "duplicate";
    }
    return "unknown";
}


// History entry for undo support.
struct ActionHistoryEntry {
    std::wstring file_path;
    ActionType action_type = ActionType::Rename;
    std::string old_value;
    std::string new_value;
    std::wstring backup_path;
    bool performed = true;
};

</content>
<write_to_file>
<path>src/organization/UndoManager.h</path>
<content>#pragma once
#include <vector>
#include "../core/ActionModel.h"

class UndoManager {
public:
    static void record_action(const ActionHistoryEntry& entry);
    static void undo(int count = 1);
    static std::vector<ActionHistoryEntry> get_history();

private:
    static std::vector<ActionHistoryEntry> history_;
};

</content>
<write_to_file>
<path>src/organization/OrganizationSvc.h</path>
<content>#pragma once
#include <vector>
#include "../core/ActionModel.h"
#include "RenameAction.h"
#include "MoveAction.h"
#include "DeleteAction.h"
#include "SymlinkAction.h"
#include "ArchiveAction.h"

class OrganizationSvc {
public:
    static std::vector<ActionItem> generate_actions(const DuplicateGroup& group, ActionType action_type);
    static std::vector<ActionItem> generate_actions(const std::vector<DuplicateGroup>& groups,
                                                    ActionType action_type = ActionType::Rename);
    static std::wstring generate_renamed_path(const FileInfo& file, int index);
    static void apply(std::vector<ActionItem>& items);
    static void apply_actions(const std::vector<ActionItem>& items);
    static void undo_actions();

private:
    static std::vector<ActionHistoryEntry> history_;
};

</content>
<write_to_file>
<path>plan.md</path>
<content># DupeCheck — Development Plan

## Overview

DupeCheck is a C++ Windows application that finds duplicate files across folders/drives using multi-tier hashing (XxHash32 + SHA256) and provides batch organization actions. The app runs as both a foreground GUI (ImGui-based) and an installable Windows Service with SQLite-backed caching.

---

## 1. Technology Stack

| Layer | Choice |
|-------|--------|
| Language | C++20 |
| Build System | CMake 3.24+ |
| GUI | ImGui + Dear ImGui Win32 backend (single `.exe`) |
| Hashing | Local XxHash32 implementation (`src/hashing/xxhash.c/h`), Windows native SHA256 via **Bcrypt** API (BCryptOpenAlgorithmProvider, BCryptHashData) |
| Database | sqlite3.h (compiled in or linked as `sqlite3.lib`) with WAL mode for multi-process concurrency |
| Service | Native Windows Service API (WinMain-based service) |
| Threading | `<thread>`, `<mutex>`, C++ thread pools |
| File I/O | Win32 APIs (`CreateFileW`, `ReadFile`), buffering for performance |

### External Dependencies (bundled as source or pre-built libs)
- **ImGui** — single header + Win32 backend
- **SQLite** — amalgamation source (compiled into the project)
- **XxHash** — developed locally as part of the application, not an external dependency (`src/hashing/xxhash.c` / `src/hashing/xxhash.h`)

---

## 2. Architecture Overview

```
┌─────────────────────────────────────────────────┐
│                  DupeCheck.exe                   │
│                                                  │
│  ┌──────┬───────┐    ┌──────────┬────────────┐   │
│  │  View │ Core Lib│    │ Service  │ GUI        │   │
│  │ Layer │ Layer   │◄──│ IPC/API  │ Manager    │   │
│  └──────┴───────┘    └──────────┴────────────┘   │
│         │                       │                │
│  ┌──────▼───────────────────────▼──────────────┐ │
│  │              Core Library                    │ │
│  │                                              │ │
│  │  ┌────────────┐ ┌────────────────────┐       │ │
│  │  │ FileScanner│ │ CachedScannerSvc   │       │ │
│  │  │ (enumerate,│ │ (SQLite cache,      │       │ │
│  │  │  XxHash32) │ │  incremental scan)  │       │ │
│  │  └────┬───────┘ └────────┬───────────┘       │ │
│  │  ┌────▼──────────────────▼──────────┐        │ │
│  │  │     Duplicate Engine             │         │ │
│  │  │ (strategies, matching, grouping) │         │ │
│  │  └────┬─────────────────────┬──────┘        │ │
│  │       │                     │               │ │
│  │  ┌────▼────────┐    ┌──────▼───────────┐    │ │
│  │  │ HashEngine  │    │ OrganizationSvc   │    │ │
│  │  │ (SHA256,    │    │ (rename/move/     │    │ │
│  │  │  XxHash32)  │    │ delete/symlink/   │    │ │
│  │  └─────────────┘    │ archive/undo)     │    │ │
│  │                     └───────────────────┘    │ │
│  └──────────────────────────────────────────────┘ │
└───────────────────────────────────────────────────┘
         ▲                    ▲
         │                    │
   ┌─────┴────────────────┐  │
   │   SQLite Database    │◄─┘
   │   %APPDATA%\DupeCheck│
   │   dupecheck.db       │
   └──────────────────────┘
```

### Module Responsibilities

| Module | Responsibility |
|--------|---------------|
| `FileScanner` | Enumerates a directory tree recursively; collects file metadata and computes XxHash32 in parallel workers |
| `HashEngine` | Computes XxHash32 + SHA256 in a single I/O pass per file using 64KB read buffers; thread pool (N = CPU cores - 1) |
| `DuplicateEngine` | Runs all enabled detection strategies and merges results |
| `CachedScannerService` | Persists scan results in SQLite with WAL mode; supports incremental updates by comparing file metadata against cached entries |
| `OrganizationSvc` | Executes batch actions on duplicate groups; builds a history log for undo support |
| `ImGuiView` | Renders the main window, controls (path input, scan button), result panels, preview/apply dialogs, and settings panel using Dear ImGui Win32 backend |

---

## 3. Core Data Model

```cpp
struct FileInfo {
    std::wstring path;          // Full file path
    uint64_t size;              // File size in bytes
    long long mtime;            // Last write time, seconds since epoch (UTC)
    XxHash32 xxhash;            // Tier-1 hash (computed during scan)
    Sha256 sha256;              // Tier-2 hash (for confirmed matches)
};

struct DuplicateGroup {
    std::vector<FileInfo> files;       // Files in this group
    Strategy strategy;                 // Which duplicate detection matched them
    std::string label;                  // Human-readable description
};

enum class Strategy : uint32_t {
    ExactMatch = 1,         // SHA256 match
    NameVariant = 2,        // Same content + name within Levenshtein distance
    SizeHashSimilar = 4,    // Similar size + XxHash in same bin
    ExtensionFamily = 8,    // Same content across extension family
    FolderCopy = 16,        // Entire directory trees copied
};

enum class ActionType {
    Rename,
    MoveToDuplicatesFolder,
    Delete,
    CreateSymlink,
    Archive,
};

struct ActionItem {
    FileInfo file;
    FileType type;            // "original" vs "duplicate" (for rename)
    std::string new_name;     // Proposed new name
    int copy_index;           // Index for generating renamed path
    ActionType action;
    bool selected;            // Whether user has checked this action
};
```

---

## 4. Detection Strategies

### 4a. Exact Match — Strategy value: **1**
Group files by SHA256 hash. Any group with ≥2 entries is a duplicate set. Fastest strategy, zero false positives.

### 4b. Name Variant — Strategy value: **2**
For each pair of files with identical content (same SHA256), compute the Levenshtein distance between file names (without extension). If `distance <= nameSimilarityThreshold` (default: 3), classify as a "Name Variant". Handles cases like `report_1.docx`, `report_2.docx`.

### 4c. Size+Hash Similar — Strategy value: **4**
Bin files by exact size first. Within each size bin, group by XxHash32 range using `[hash & ~(tolerance - 1)]` where tolerance defaults to 1024 bytes. Detects modified copies of similar-sized files.

### 4d. Extension Family — Strategy value: **8**
Map extensions to families (`{jpg, jpeg, jpe} → "image"`, `{docx, doc, docm} → "document"`). Files in the same family with matching SHA256 are duplicates (e.g., `photo.jpg` and `photo.jpeg`).

### 4e. Folder Copy — Strategy value: **16**
For each directory, compute a tree hash via SHA256 over sorted `name|size\n` entries. Group directories by tree hash; groups with ≥2 entries = folder copies. Directory-level trees are computed inline using the shared Bcrypt provider from `HashEngine`.

---

## 5. SQLite Database Design

Location: `%APPDATA%\DupeCheck\dupecheck.db`

### Schema

```sql
-- Open in WAL mode for multi-process safety (service + GUI)

CREATE TABLE IF NOT EXISTS files (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    path TEXT UNIQUE NOT NULL COLLATE NOCASE,
    size BIGINT NOT NULL,
    mtime BIGINT NOT NULL,  -- seconds since epoch
    xxhash32 BIGINT NOT NULL,
    sha256 BLOB(32) NOT NULL,
    last_scan INTEGER NOT NULL  -- filemtime in seconds at last scan
);

CREATE TABLE IF NOT EXISTS scan_sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    path_hash BIGINT NOT NULL DEFAULT 0,
    scan_path TEXT NOT NULL,
    created_at BIGINT NOT NULL DEFAULT (strftime('%s', 'now')),
    file_count INTEGER NOT NULL,
    duplicate_count INTEGER NOT NULL,
    strategy_flags INTEGER NOT NULL  -- bitmask of enabled strategies
);

CREATE TABLE IF NOT EXISTS action_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    session_id INTEGER REFERENCES scan_sessions(id),
    file_path TEXT NOT NULL,
    action_type TEXT NOT NULL,       -- 'rename', 'move', 'delete', etc.
    old_value TEXT,                  -- e.g., old path or name
    new_value TEXT,                  -- e.g., new path or name
    performed_at BIGINT NOT NULL DEFAULT (strftime('%s', 'now'))
);
```

### Incremental Scan Algorithm

```
For each file on disk:
    IF (size == cached.size AND mtime == cached.mtime):
        skip re-hashing; use cached hashes.
    ELSE IF file exists but not in cache:
        INSERT INTO files (path, size, mtime, xxhash32, sha256) VALUES (...)
    ELSE:
        UPDATE files SET xxhash32=?, sha256=? WHERE path=?

For each file in cache NOT on disk:
    DELETE FROM files WHERE path=?
```

---

## 6. Organization Actions

### 6a. Rename
- For each duplicate group, assign sequential suffixes (e.g., `photo.jpg` → `photo (copy).jpg`, `photo (copy 2).jpg`).

### 6b. Move to Duplicate Folders
- Create `<original_dir>/duplicates/` and move copies there.

### 6c. Delete Copies
- Keep the "first" file, delete all others in the group. Note: undo recreates a marker file (the original content is not preserved by default).

### 6d. Symlink
- Replace each duplicate with a symlink pointing back to the original.
- Uses `CreateSymbolicLinkW`. The symlink is named `_filename.link` in the same directory as the original.

### 6e. Archive
- Copy files using Windows Shell API (`SHFileOperationW`). A placeholder ZIP archive can also be created via `create_zip()` which writes a minimal PK header to disk.

### Undo Mechanism
- Each action records `(file_path, old_value, new_value, action_type)` in the history stack and optionally in `action_history`.
- The **Undo** button replays actions in reverse order: e.g., `delete → move_back`, `rename → rename_back`.

---

## 7. Settings Configuration

Stored as JSON at `%APPDATA%\DupeCheck\settings.json` (or persisted to SQLite):

```json
{
    "name_similarity_threshold": 3,
    "hash_tolerance": 1024,
    "allow_cross_strategy": true,
    "max_concurrent_hashers": 7,
    "buffer_size_kb": 64,
    "extension_families": {
        "image": ["jpg", "jpeg", "jpe", "png", "gif"],
        "document": ["docx", "doc", "docm"]
    },
    "default_rename_format": "{name} ({count}){ext}",
    "service_enabled": true,
    "service_scan_path": ""
}
```

---

## 8. Windows Service

### Installation (`--install-service <path>`)
1. Copy self to `%APPDATA%\DupeCheck\dupecheck.exe`.
2. Register with `CreateServiceW` as a service named `DupeCheck`.
3. Set startup type to `Automatic`.
4. Start the service; it loads its path and scan_path from arguments + settings.

### Service Behavior
- Runs in the foreground of its own process (no separate daemon).
- Periodically re-scans the configured path.
- Updates SQLite database with incremental changes.
- Listens for commands via named pipe (`\\.\pipe\dupecheck`) for:
  - `SCAN` — trigger a scan immediately
  - `GET_RESULTS` — return latest results as JSON
  - `PERFORM_ACTIONS` — execute batch actions from the GUI

### Client Communication
The GUI connects to the service via:
- Direct SQLite read (if service owns the DB, use WAL mode) **OR**
- Named pipe RPC for real-time command/response.

---

## 9. GUI Design (ImGui)

### Main Window Layout

```
┌──────────────────────────────────────────────────────┐
│ ┌─────┐ [ C:\Your\Path\____________ ] [📁] [Scan]   │
│ ├─────┼─────────────────────────────────────────────┤ │
│ │ D   │  ● Exact Match (3 groups, 9 files)           │ │
│ │ U   │    [▶ Preview] [▼ Details]                   │ │
│ │ P   │                                              │ │
│ L   │  ● Name Variants (2 groups, 4 files)           │ │
│ Y   │    [▶ Preview]                                   │ │
│ T   │  ● Size+Hash Similar (1 group)                 │ │
│   │    [▶ Preview]                                   │ │
│   │                                              │ │
│   │  ● Extension Family                            │ │
│   │    [▶ Preview]                                   │ │
│   │                                              │ │
│   │  ● Folder Copy                                 │ │
│   │    [▶ Preview]                                   │ │
└───┴────────────────────────────────────────────────────┘
┌────────────────────────────────────────────────────────────┐
│ Preview: Rename Actions                                    │
│ ┌──────────────────────────────────────────────────────────┐│
│ │ ☑ report_1.docx → report (copy 1).docx                  ││
│ │ ☐ report_2.docx → report (copy 2).docx                  ││
│ │ ☑ photo.jpg → photo (copy).jpg                          ││
│ └──────────────────────────────────────────────────────────┘│
│ [Apply All]  [Undo Last]          [Settings]               │
└────────────────────────────────────────────────────────────┘
```

### Panels and Controls
- **Path Input** — Editable text field + folder picker button. Supports drive paths like `C:\`.
- **Scan Button** — Triggers scan; shows progress bar during scanning.
- **Strategy Tabs** — Expand/collapse per strategy type.
- **Preview Panel** — Shows proposed actions (rename, move, delete). Checkboxes per file.
- **Apply Button** — Executes all checked actions; updates history log.
- **Undo Button** — Reverses the last batch of actions.
- **Settings Dialog** — Modal with sliders for thresholds, extension family editor, service toggle.

### Settings Dialog Fields
| Field | Type | Default |
|-------|------|---------|
| Name Similarity Threshold | Slider (0–10) | 3 |
| Hash Tolerance | SpinBox (256–4096) | 1024 |
| Max Concurrent Hashers | SpinBox (1–CPU cores-1) | Auto |
| Extension Families | Multi-line editor | See settings schema |
| Enable Windows Service | Checkbox | true |

---

## 10. Project Structure

```
dupecheck/
├── CMakeLists.txt                  # Top-level CMake, sets up subprojects
├── src/
│   ├── main.cpp                    # Entry point (WinMain for GUI mode)
│   ├── cli.cpp                     # CLI argument parsing (--install-service, --uninstall-service)
│   │
│   ├── core/
│   │   ├── FileInfo.h              # FileInfo struct definitions + PathUtils namespace
│   │   ├── Strategy.h              # Strategy enum + detection config
│   │   └── ActionModel.h           # ActionItem, ActionType definitions
│   │
│   ├── hashing/
│   │   ├── HashEngine.cpp          # Single-pass XxHash32+SHA256 computation
│   │   ├── HashEngine.h
│   │   ├── ThreadPool.cpp/h        # Fixed-size thread pool with work queue
│   │   └── xxhash/                 # Local XxHash implementation (xxhash.c/.h)
│   │
│   ├── scanner/
│   │   ├── FileScanner.cpp         # Directory enumeration + initial hashing
│   │   ├── CachedScannerService.cpp# SQLite-backed incremental scan
│   │
│   ├── engine/
│   │   ├── DuplicateEngine.cpp     # Strategy dispatching and result merging
│   │   ├── ExactMatch.h            # SHA256 match
│   │   ├── NameVariant.h           # Levenshtein name similarity
│   │   ├── SizeHashSimilar.h       # XxHash binning strategy
│   │   ├── ExtensionFamily.h       # Extension family mapping
│   │   └── FolderCopy.h            # Directory tree hashing (inline)
│   │
│   ├── organization/
│   │   ├── OrganizationSvc.cpp     # Batch action executor + history
│   │   ├── RenameAction.h          # Rename with configurable format strings
│   │   ├── MoveAction.h            # Move-to-duplicates-folder
│   │   ├── DeleteAction.h          # Delete copies
│   │   ├── SymlinkAction.h         # CreateSymbolicLinkW wrapper
│   │   ├── ArchiveAction.h         # Zip/archive duplicates (using Windows API)
│   │   └── UndoManager.h           # History stack for undo support
│   │
│   ├── database/
│   │   ├── DatabaseManager.cpp     # SQLite CRUD, WAL mode
│   │
│   ├── service/
│   │   ├── ServiceHost.cpp         # Windows Service main loop + CLI
│   │   └── NamedPipeServer.cpp     # Pipe-based IPC
│   │
│   ├── gui/
│   │   ├── ImGuiView.cpp           # Main window rendering loop
│   │   ├── Controls.cpp            # Path input, buttons, progress bar
│   │   ├── PreviewPanel.cpp        # Action preview widget
│   │   └── SettingsDialog.cpp      # Settings modal
│   │
│   └── utils/
│       ├── JsonConfig.cpp          # Lightweight JSON config reader/writer (UTF-8)
│       ├── Levenshtein.h           # Templated Levenshtein distance algorithm
│       └── ExtensionFamilyMap.h    # Built-in extension family mappings (cached)
│
├── external/                       # External deps bundled as source or submodules
│   ├── imgui/                    # Dear ImGui source files + headers
│   └── sqlite3/                  # SQLite amalgamation (sqlite3.c/.h)
│
└── tests/                          # Unit tests (GoogleTest)
    ├── test_levenshtein.cpp        # Levenshtein distance tests
    ├── test_hash_engine.cpp        # XxHash32 + SHA256 correctness
    └── test_duplicate_engine.cpp   # Strategy detection unit tests
```

---

## 11. Build System (CMake)

### Top-Level `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.24)
project(DupeCheck LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_SERVICE "Build Windows service binary" ON)

add_subdirectory(external/imgui)     # Dear ImGui source files
add_library(xxhash STATIC src/hashing/xxhash/xxhash.c)
target_include_directories(xxhash PUBLIC ${CMAKE_SOURCE_DIR})

add_library(sqlite3 STATIC external/sqlite3/sqlite3.c)
target_include_directories(sqlite3 PRIVATE ${CMAKE_SOURCE_DIR}/external/sqlite3)

# Core library
add_library(dupecheck_core STATIC ...)
target_link_libraries(dupecheck_core PRIVATE sqlite3 xxhash imgui bcrypt)

# GUI executable
add_executable(dupecheck WIN32 src/main.cpp src/cli.cpp ...)
target_link_libraries(dupecheck PRIVATE dupecheck_core)

# Service (console-subsystem binary)
if(BUILD_SERVICE)
    add_executable(dupecheck_service src/service/ServiceHost.cpp ...)
    target_link_libraries(dupecheck_service PRIVATE dupecheck_core msvcrt kernel32 advapi32)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_executable(tests tests/*.cpp)
    target_link_libraries(tests PRIVATE dupecheck_core gtest gtest_main)
    add_test(NAME UnitTests COMMAND tests)
endif()
```

---

## 12. Development Phases & Milestones

### Phase 0: Foundation (Weeks 1–2)
- [ ] Set up CMake project structure
- [ ] Bundle ImGui, sqlite3, xxhash sources
- [ ] Implement basic `FileInfo` model and path utilities
- [ ] Create Win32 + ImGui "Hello World" window

### Phase 1: Hashing Engine (Weeks 2–4)
- [X] Develop local XxHash32 implementation in `src/hashing/xxhash.c/.h` as part of the application source tree
- [X] SHA256 via Bcrypt API (`BCryptOpenAlgorithmProvider`, `BCryptHashData`)
- [X] ThreadPool with work-stealing
- [ ] Single-pass computation

### Phase 2: File Scanner & Caching (Weeks 4–6)
- [ ] `FileScanner` enumerates directories recursively (`FindFirstFileW` / `FindNextFileW`)
- [XxHash] Parallel hash computation during enumeration
- [SQLite] Create `DatabaseManager` with schema initialization
- [Incremental] Implement incremental scan logic (compare size+mtime)

### Phase 3: Duplicate Detection Engine (Weeks 6–9)
- [ ] Strategy dispatching in `DuplicateEngine`
- [X] All five strategies implemented and tested

### Phase 4: Organization Actions (Weeks 9–12)
- [ ] `OrganizationSvc` orchestrates batch actions
- [ ] Rename, MoveToDuplicatesFolder, Delete, Symlink, Archive
- [ ] UndoManager with reverse action support

### Phase 5: GUI Implementation (Weeks 12–16)
- [ ] Main window layout with ImGui docking
- [ ] Path input + folder picker button
- [ ] Scan progress bar during scanning
- [ ] Strategy expand/collapse panels with result counts
- [ ] Preview panel showing proposed actions per group
- [ ] Apply/Undo buttons connected to `OrganizationSvc`

### Phase 6: Windows Service (Weeks 16–18)
- [ ] `ServiceHost.cpp` — registers as a Windows service
- [X] Named pipe server for IPC between GUI and service
- [ ] CLI `--install-service <path>` / `--uninstall-service` commands

### Phase 7: Polish & Testing (Weeks 18–20)
- [ ] Error handling for edge cases
- [ ] Performance testing on large drives
- [ ] Unicode path support (UTF-16 all the way through)
- [ ] Settings persistence and migration between versions

---

## Summary

This plan covers DupeCheck as a C++20 application with:

1. A **CMake-based build system** bundling ImGui, SQLite, and XxHash locally
2. A **dual-mode architecture** supporting both GUI and Windows Service from the same `.exe`
3. **Multi-tier hashing** (XxHash32 → SHA256) for fast duplicate detection
4. **Five detection strategies** with configurable thresholds
5. An **SQLite cache** enabling incremental scans on large drives
6. A rich **ImGui-based GUI** with preview, batch actions, and undo support
7. A **Windows Service** with CLI installation and named-pipe IPC

Estimated timeline: ~20 weeks from foundation to polished release.