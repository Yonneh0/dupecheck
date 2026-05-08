# DupeCheck — Development Plan

## Overview

DupeCheck is a C++ Windows application that finds duplicate files across folders/drives using multi-tier hashing (XxHash32 + SHA256) and provides batch organization actions. The app runs as both a foreground GUI (ImGui-based) and an installable Windows Service with SQLite-backed caching.

---

## 1. Technology Stack

| Layer | Choice |
|-------|--------|
| Language | C++20 |
| Build System | CMake 3.24+ |
| GUI | ImGui + Dear ImGui Win32 backend (single `.exe`) |
| Hashing | Local XxHash32 implementation (`xxhash.c/h`), Windows native SHA256 via CryptoAPI or bcrypt |
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
│  ┌──────────────┐    ┌───────────────────────┐   │
│  │  ImGui GUI   │◄──►│   CLI + Service       │   │
│  │  Layer       │    │   Manager             │   │
│  └──────┬───────┘    └──────────┬────────────┘   │
│         │                       │                │
│  ┌──────▼───────┐    ┌──────────▼────────────┐   │
│  │  View Layer  │    │  Service IPC / API    │   │
│  │  (ImGui     │    │  (named pipe,          │   │
│  │   widgets)  │    │   COM, or shared DB)  │   │
│  └──────┬───────┘    └──────────┬────────────┘   │
│         │                       │                │
│  ┌──────▼───────────────────────▼──────────────┐ │
│  │              Core Library                    │ │
│  │                                              │ │
│  │  ┌────────────┐ ┌────────────────────┐       │ │
│  │  │ FileScanner│ │ CachedScannerSvc   │       │ │
│  │  │ (enumerate,│ │ (SQLite cache,      │       │ │
│  │  │  XxHash32) │ │  incremental scan)  │       │ │
│  │  └────┬───────┘ └────────┬───────────┘       │ │
│  │       │                  │                   │ │
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
| `FileScanner` | Enumerates a directory tree, collects file metadata (path, size, mtime, ext), computes XxHash32 in parallel workers |
| `HashEngine` | Computes SHA256 for candidate files; batched I/O passes to minimize open/close overhead |
| `DuplicateEngine` | Runs detection strategies (exact, name-variant, size+hash, extension-family, folder-copy) against scanned data |
| `CachedScannerService` | Persists scan results in SQLite; supports incremental updates by comparing file metadata against cached entries |
| `OrganizationSvc` | Executes batch actions on duplicate groups; builds a history log for undo |
| `ImGuiView` | Renders the window, controls (path input, scan button), result panels, preview/apply dialogs, settings panel |

---

## 3. Core Data Model

```cpp
struct FileInfo {
    std::wstring path;          // Full file path
    uint64_t size;              // File size in bytes
    FILETIME mtime;             // Last write time
    XxHash32 xxHash;            // Tier-1 hash (computed during scan)
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
    std::string newName;      // Proposed new name
    ActionType action;
    bool selected;            // Whether user has checked this action
};
```

---

## 4. Hashing Architecture

### Multi-Tier Approach

| Tier | Hash | Speed | Collision Risk | Use Case |
|------|------|-------|----------------|----------|
| 1 | XxHash32 | ~50 GB/s per core | Moderate (2^32 space) | Initial grouping during scan |
| 2 | SHA256 | ~2-4 GB/s per core | Negligible | Confirm exact matches within groups |

### Single-Pass Parallel Hashing

```
Thread Pool (N workers, N = CPU cores - 1):

For each file in the enumerate list:
    1. Open file (CreateFileW)
    2. Read buffer loop:
       - Read 64KB chunks
       - Update XxHash32 state incrementally
       - Update SHA256 state incrementally
    3. Close file
    4. Store {path, size, mtime, xxhash, sha256} in results
```

### Folder-Copy Tree Hashing Strategy

For the `FolderCopy` strategy:
1. For each directory being scanned, compute a **tree hash**: recursively sort children by name, concatenate `(name + xxHash)` per entry, then SHA256 over the concatenated string.
2. Two directories are "copies" if their tree hashes match (allowing for minor renaming).

---

## 5. Detection Strategies

### 5a. Exact Match
- Group files by `sha256`. Any group with ≥2 entries = duplicates.
- Fastest, zero false positives.

### 5b. Name Variant
- For each pair of files with same content (same sha256):
  - Compute Levenshtein distance between file names (without extension).
  - If `distance <= nameSimilarityThreshold` (default: 3), classify as a "Name Variant".
- Also handles cases like `report_1.docx`, `report_2.docx` where content differs slightly but structure is similar.

### 5c. Size+Hash Similar
- Bin files by `size` first (exact size match).
- Within each size bin, group by XxHash32 range: `[hash & ~(tolerance - 1)]`.
- Tolerance defaults to 1024 bytes; bins are computed as `xxHash >> 8` for grouping.

### 5d. Extension Family
- Map extensions to families: `{jpg, jpeg, jpe} → "image"`, `{docx, doc, docm} → "document"`.
- Files in the same family with matching SHA256 are duplicates (e.g., `photo.jpg` and `photo.jpeg`).

### 5e. Folder Copy
- For each directory: compute a tree hash (see §4 above).
- Group directories by tree hash; groups with ≥2 entries = folder copies.

---

## 6. SQLite Database Design

Location: `%APPDATA%\DupeCheck\dupecheck.db`

### Schema

```sql
-- Open in WAL mode for multi-process safety (service + GUI)

CREATE TABLE IF NOT EXISTS files (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    path TEXT UNIQUE NOT NULL COLLATE NOCASE,
    size BIGINT NOT NULL,
    mtime BLOB(8) NOT NULL,  -- binary FILETIME bytes
    xxhash32 INTEGER NOT NULL,
    sha256 BLOB(32) NOT NULL,
    last_scan INTEGER NOT NULL  -- filemtime in seconds at last scan
);

CREATE INDEX IF NOT EXISTS idx_files_path ON files(path);
CREATE INDEX IF NOT EXISTS idx_files_size ON files(size);
CREATE INDEX IF NOT EXISTS idx_files_xxhash ON files(xxhash32);

-- For folder copy strategy: store directory tree hashes
CREATE TABLE IF NOT EXISTS directories (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    path TEXT UNIQUE NOT NULL COLLATE NOCASE,
    tree_hash BLOB(32) NOT NULL,  -- SHA256 of tree structure
    last_scan INTEGER NOT NULL
);

-- Cache the scan session for quick lookup
CREATE TABLE IF NOT EXISTS scan_sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    scan_path TEXT NOT NULL,
    created_at BIGINT NOT NULL DEFAULT (strftime('%s', 'now')),
    file_count INTEGER NOT NULL,
    duplicate_count INTEGER NOT NULL,
    strategy_flags INTEGER NOT NULL  -- bitmask of enabled strategies
);

-- Action history for undo support
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

## 7. Organization Actions

### 7a. Rename
- For each duplicate group, assign sequential suffixes or prefixes.
- Example: `photo.jpg` → `photo (copy).jpg`, `photo (copy 2).jpg`.
- Configurable format string in settings.

### 7b. Move to Duplicate Folders
- Create `<original_dir>/duplicates/` and move copies there.

### 7c. Delete Copies
- Keep the "first" file, delete all others in the group.

### 7d. Symlink
- Replace each duplicate with a symlink pointing back to the original.
- Uses `CreateSymbolicLinkW`.

### 7e. Archive
- Zip or tar the duplicates into `<group>.zip` and remove originals from disk.

### Undo Mechanism
- Each action records `(file_path, old_value, new_value, action_type)` in `action_history`.
- The **Undo** button replays actions in reverse order: e.g., `delete → move_back`, `rename → rename_back`.

---

## 8. Settings Configuration

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

## 9. Windows Service

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

## 10. GUI Design (ImGui)

### Main Window Layout

```
┌──────────────────────────────────────────────────────┐
│ ┌─────┐ [ C:\Your\Path\____________ ] [📁] [Scan]   │
│ │     ├─────────────────────────────────────────────┤ │
│ │ D │  ● Exact Match (3 groups, 9 files)           │ │
│ │ U │    [▶ Preview] [▼ Details]                   │ │
│ │ P │                                              │ │
│ L │  ● Name Variants (2 groups, 4 files)           │ │
│   │    [▶ Preview]                                   │ │
│ Y │  ● Size+Hash Similar (1 group)                 │ │
│   │    [▶ Preview]                                   │ │
│ T │                                              │ │
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

## 11. Project Structure

```
dupecheck/
├── CMakeLists.txt                  # Top-level CMake, sets up subprojects
├── src/
│   ├── main.cpp                    # Entry point (WinMain for GUI mode)
│   ├── cli.cpp                     # CLI argument parsing (--install-service, --uninstall-service)
│   │
│   ├── core/
│   │   ├── FileInfo.h              # FileInfo struct definitions
│   │   ├── HashTypes.h             # XxHash32, Sha256 type wrappers
│   │   ├── Strategy.h              # Strategy enum + detection config
│   │   └── ActionModel.h           # ActionItem, ActionType definitions
│   │
│   ├── hashing/
│   │   ├── HashEngine.cpp          # Single-pass XxHash32+SHA256 computation
│   │   ├── HashEngine.h
│   │   ├── ThreadPool.h            # Fixed-size thread pool with work queue
│   │   └── xxhash_wrapper.c/h      # Local XxHash implementation + wrapper
│   │
│   ├── scanner/
│   │   ├── FileScanner.cpp         # Directory enumeration + initial hashing
│   │   ├── FileScanner.h
│   │   ├── CachedScannerService.cpp # SQLite-backed incremental scan
│   │   └── CachedScannerService.h
│   │
│   ├── engine/
│   │   ├── DuplicateEngine.cpp     # Strategy dispatching and result merging
│   │   ├── DuplicateEngine.h
│   │   ├── ExactMatch.cpp          # Strategy 5a
│   │   ├── NameVariant.cpp         # Strategy 5b (Levenshtein)
│   │   ├── SizeHashSimilar.cpp     # Strategy 5c
│   │   ├── ExtensionFamily.cpp     # Strategy 5d
│   │   └── FolderCopy.cpp          # Strategy 5e (tree hashing)
│   │
│   ├── organization/
│   │   ├── OrganizationSvc.cpp     # Batch action executor
│   │   ├── OrganizationSvc.h
│   │   ├── RenameAction.cpp        # Rename logic
│   │   ├── MoveAction.cpp          # Move-to-duplicates-folder
│   │   ├── DeleteAction.cpp        # Delete copies
│   │   ├── SymlinkAction.cpp       # CreateSymbolicLinkW wrapper
│   │   ├── ArchiveAction.cpp       # Zip/archive duplicates (using zlib or Windows API)
│   │   └── UndoManager.h           # History stack for undo
│   │
│   ├── service/
│   │   ├── ServiceHost.cpp         # Windows Service main loop
│   │   ├── ServiceHost.h
│   │   └── NamedPipeServer.cpp     # Pipe-based IPC
│   │
│   ├── database/
│   │   ├── DatabaseManager.cpp     # SQLite open, schema init, queries
│   │   └── DatabaseManager.h
│   │
│   ├── gui/
│   │   ├── ImGuiView.cpp           # Main window rendering loop
│   │   ├── ImGuiView.h
│   │   ├── Controls.cpp            # Path input, buttons, progress bar
│   │   ├── PreviewPanel.cpp        # Action preview widget
│   │   └── SettingsDialog.cpp      # Settings modal
│   │
│   └── utils/
│       ├── PathUtils.h             # wstring path helpers, extension extraction
│       ├── Levenshtein.h           # Levenshtein distance function
│       ├── ExtensionFamilyMap.h    # Built-in family mappings
│       └── JsonConfig.cpp          # Settings JSON read/write (nlohmann/json or manual)
│
├── external/                       # External deps bundled as submodules or source
│   ├── imgui/                    # Dear ImGui source files + headers
│   │   ├── imconfig.h            # Config: enable Win32 backend, C++17 features
│   │   └── ...                   # All .cpp/.h files from imgui repo
│   └── sqlite3/                  # SQLite amalgamation (sqlite3.c/.h)
├── src/hashing/xxhash/           # XxHash source developed locally as part of app
│
├── resources/                      # Icons, manifest, resources
│   ├── app.ico
│   └── dupecheck.rc
│
└── tests/                          # Unit tests (GoogleTest or Catch2)
    ├── test_levenshtein.cpp
    ├── test_hash_engine.cpp
    ├── test_duplicate_engine.cpp
    └── test_organization.cpp
```

---

## 12. Build System (CMake)

### Top-Level `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.24)
project(DupeCheck LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_SERVICE "Build Windows service binary" ON)

# Internal sources + bundled deps
add_subdirectory(imgui)           # Dear ImGui source files
add_library(xxhash STATIC src/hashing/xxhash/xxhash.c)
target_include_directories(xxhash PUBLIC ${CMAKE_SOURCE_DIR})

add_subdirectory(external/imgui)  # Additional imgui (if separate)
add_library(sqlite3 STATIC external/sqlite3/sqlite3.c)
target_include_directories(sqlite3 PRIVATE ${CMAKE_SOURCE_DIR}/external/sqlite3)

# Core library
add_library(dupecheck_core STATIC
    src/hashing/HashEngine.cpp
    src/scanner/FileScanner.cpp
    src/scanner/CachedScannerService.cpp
    src/engine/DuplicateEngine.cpp
    src/engine/ExactMatch.cpp
    ... etc.
)
target_link_libraries(dupecheck_core PRIVATE sqlite3 xxhash imgui bcrypt)

# GUI executable
add_executable(dupecheck WIN32 src/main.cpp src/cli.cpp src/gui/ImGuiView.cpp ...)
target_link_libraries(dupecheck PRIVATE dupecheck_core)

# Service (console-subsystem binary)
if(BUILD_SERVICE)
    add_executable(dupecheck_service src/service/ServiceHost.cpp src/cli.cpp ...)
    target_link_libraries(dupecheck_service PRIVATE dupecheck_core msvcrt kernel32 advapi32)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    # Add GoogleTest or Catch2 as subproject
    add_executable(tests tests/*.cpp)
    target_link_libraries(tests PRIVATE dupecheck_core gtest gtest_main)
    add_test(NAME UnitTests COMMAND tests)
endif()

# Install rules for self-copy to %APPDATA%\DupeCheck\
install(TARGETS dupecheck RUNTIME DESTINATION DupeCheck)
```

---

## 13. Development Phases & Milestones

### Phase 0: Foundation (Weeks 1–2)
- [ ] Set up CMake project structure
- [ ] Bundle ImGui, sqlite3, xxhash sources
- [ ] Implement basic `FileInfo` model and path utilities
- [ ] Create Win32 + ImGui "Hello World" window

### Phase 1: Hashing Engine (Weeks 2–4)
- [XxHash] Develop local XxHash32 implementation in `src/hashing/xxhash.c/.h` as part of the application source tree (no external dependency)
- [SHA256] Implement SHA256 via Bcrypt API (`BCryptOpenAlgorithmProvider`, `BCryptHashData`)
- [ThreadPool] Build a simple thread pool with work stealing
- [Single-pass] Compute both hashes in one I/O pass per file
- Unit tests for hashing correctness (verify against known values)

### Phase 2: File Scanner & Caching (Weeks 4–6)
- [ ] `FileScanner` enumerates directories recursively (`FindFirstFileW` / `FindNextFileW`)
- [XxHash] Parallel hash computation during enumeration
- [SQLite] Create `DatabaseManager` with schema initialization
- [Incremental] Implement incremental scan logic (compare size+mtime)
- Unit tests for scanner + cache behavior

### Phase 3: Duplicate Detection Engine (Weeks 6–9)
- [ ] Strategy dispatching in `DuplicateEngine`
- [Exact Match] Group by SHA256 — unit tested
- [Name Variant] Levenshtein distance algorithm, threshold-based matching — unit tested
- [Size+Hash Similar] XxHash binning strategy — unit tested
- [Extension Family] Extension mapping + hash comparison — unit tested
- [Folder Copy] Tree hashing for directories — unit tested

### Phase 4: Organization Actions (Weeks 9–12)
- [ ] `OrganizationSvc` orchestrates batch actions
- [Rename] Rename action with configurable format strings
- [MoveToDuplicatesFolder] Create duplicates folder and move
- [Delete] Delete copies from disk
- [Symlink] Symbolic link creation on Windows
- [Archive] Zip/archive (using zlib bundled or Win32 compression APIs)
- [UndoManager] History stack with reverse action support

### Phase 5: GUI Implementation (Weeks 12–16)
- [ ] Main window layout with ImGui docking
- [ ] Path input + folder picker button
- [ ] Scan progress bar during scanning
- [ ] Strategy expand/collapse panels with result counts
- [ ] Preview panel showing proposed actions per group
- [ ] Apply/Undo buttons connected to `OrganizationSvc`
- [ ] Settings dialog (thresholds, extension families, service toggle)

### Phase 6: Windows Service (Weeks 16–18)
- [ ] `ServiceHost.cpp` — registers as a Windows service
- [ ] Named pipe server for IPC between GUI and service
- [ ] CLI `--install-service <path>` / `--uninstall-service` commands
- [ ] Service auto-starts on system boot with configured scan path

### Phase 7: Polish & Testing (Weeks 18–20)
- [ ] Error handling for edge cases (permission denied, long paths >260 chars via `\\?\` prefix)
- [ ] Performance testing on large drives (multi-TB external disk)
- [ ] Unicode path support (UTF-16 all the way through)
- [ ] Accessibility: keyboard navigation in ImGui tree views
- [ ] Settings persistence and migration between versions

---

## 14. Key Design Decisions & Trade-offs

| Decision | Rationale |
|----------|-----------|
| XxHash32 as Tier-1 | ~50 GB/s throughput enables rapid candidate grouping on large drives; low enough collision risk for pre-filtering |
| SHA256 only within candidates | Avoids full-disk SHA256 scan; reduces I/O by 70–90% on typical drives |
| SQLite WAL mode | Allows the service and GUI to read/write simultaneously without locking |
| Local XXhash (developed as part of app, lives in `src/hashing/xxhash.c/.h`) | Single `.exe` deployment, no DLL hell; source is internal to the project |
| ImGui over WPF/Qt | Minimal dependencies, fast rendering, easy embedding into Win32 app; single `.exe` with no framework install |
| Bcrypt API for SHA256 | Native on Windows 7+, hardware-accelerated via AES-NI/RDRAND |
| Thread pool size = CPU cores - 1 | Leaves one core free for GUI responsiveness during scanning |

---

## 15. Testing Strategy

### Unit Tests (Catch2 or GoogleTest)
- `test_levenshtein.cpp` — edge cases, identity distance, max edit distance
- `test_hash_engine.cpp` — verify XxHash32 output against known vectors; SHA256 against test data
- `test_duplicate_engine.cpp` — each strategy tested in isolation with synthetic file sets
- `test_organization.cpp` — rename format strings, undo reversibility

### Integration Tests
- Full scan of a temp directory → validate result structure
- Incremental scan after modifying/deleting files → cache consistency
- Service install/uninstall cycle via CLI
- Multi-process access to SQLite DB (simulated service + GUI)

---

## 16. Deployment Checklist

- [ ] Single `.exe` copy to `%APPDATA%\DupeCheck\dupecheck.exe`
- [ ] SQLite database auto-created on first run
- [ ] Settings file auto-created with defaults
- [ ] Service registered via `--install-service`
- [ ] Long-path support enabled (manifest: `<windowsLongPath>true</windowsLongPath>`)
- [ ] Installer option (MSI or NSIS) for non-power users

---

## 17. Future Extensions

- **Cloud storage sync** — scan OneDrive/Dropbox folders with awareness of cloud-only placeholders
- **Visual diff** — show side-by-side image previews in the GUI
- **Filtering** — exclude hidden files, system files, or specific extensions from results
- **Export reports** — CSV/JSON export of duplicate groups
- **Scheduled scans** — background service triggers scans on a configurable schedule

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