# DupeCheck — Development Plan

## Overview

DupeCheck is a C++20 Windows application that finds duplicate files across folders/drives using multi-tier hashing (XxHash32 + SHA256) and provides batch organization actions. Runs as both GUI (.exe) and installable Windows Service with SQLite-backed caching at `%APPDATA%\DupeCheck\dupecheck.db`.

---

## Technology Stack

| Layer | Choice |
|-------|--------|
| Language | C++20, C17 |
| Build System | CMake 3.24+ |
| GUI | ImGui + Dear ImGui Win32 backend |
| Hashing | Local XxHash32 (`src/hashing/xxhash.cpp`), Windows native SHA256 via Bcrypt API |
| Database | sqlite3 with WAL mode for multi-process concurrency |
| Service | Native Windows Service API, named pipe IPC |

---

## Architecture

```
┌─────────────────────────────────────────────┐
│                  DupeCheck.exe               │
│                                              │
│  ┌──────┬───────┐    ┌──────────┬────────┐   │
│  │ View │ Core  │◄───│ Service  │ GUI     │   │
│  │ Layer │ Lib   │    │ IPC/API  │ Manager │   │
│  └──────┴───────┘    └──────────┴────────┘   │
│         │                       │             │
│  ┌──────▼──────────────────────▼────────────┐ │
│  │          Core Library                     │ │
│  │  ┌────────────┐ ┌────────────────────┐    │ │
│  │  │ FileScanner│ │ CachedScannerService│   │ │
│  │  │ (wraps     │ │ (SQLite cache)      │    │ │
│  │  │  via       │ └────────────────────┘    │ │
│  │  │  member)   │                           │ │
│  │  └────────────┘                            │ │
│  │                                           │ │
│  │  ┌────┬─────────────────┐                 │ │
│  │  │ HashEngine  │ DuplicateEngine            │ │
│  │  │ (SHA256,    │ (strategies, matching)     │ │
│  │  │  XxHash32)  │                              │ │
│  │  └────────────┘                            │ │
│  │                                           │ │
│  │  ┌──────────────────────────────────┐     │ │
│  │  │   OrganizationSvc                │      │ │
│  │  │ (rename/move/delete/symlink)    │      │ │
│  │  └──────────────────────────────────┘       │ │
│  └─────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────┘

SQLite DB: %APPDATA%\DupeCheck\dupecheck.db (WAL mode)
```

---

## Core Data Model (`src/core/ActionModel.h`)

```cpp
enum class FileType { Original, Duplicate };
enum class ActionType { Rename, MoveToDuplicatesFolder, Delete, CreateSymlink, Archive };
struct ActionItem { FileInfo file; ActionType action; bool selected; ... };
struct ActionHistoryEntry { std::wstring file_path; ActionType action_type; ... };

enum class CliCommand { None, InstallService, UninstallService, RunService };
struct ServiceArgs { std::string scan_path; bool installed = false; CliCommand command; };
```

### FileInfo (`src/core/FileInfo.h`)

```cpp
constexpr long long EPOCH_OFFSET = 13477420800LL;   // FILETIME → Unix epoch seconds
struct FileInfo {
    std::wstring path;
    uint64_t size;
    long long mtime;           // seconds since epoch (adjusted from FILETIME)
    XxHash32 xxhash;
    Sha256 sha256;
};
```

### Strategy Enum (`src/core/Strategy.h`)

| Strategy | Value | Description |
|----------|-------|-------------|
| ExactMatch | 1 | Group by SHA256 hash |
| NameVariant | 2 | Same content + name within Levenshtein distance |
| SizeHashSimilar | 4 | Binned XxHash32 comparison |
| ExtensionFamily | 8 | Same content across extension families |
| FolderCopy | 16 | Entire directory trees copied |

---

## Database Schema (WAL mode)

```sql
CREATE TABLE IF NOT EXISTS files (
    id INTEGER PRIMARY KEY AUTOINCREMENT, path TEXT UNIQUE NOT NULL COLLATE NOCASE,
    size BIGINT NOT NULL, mtime BIGINT NOT NULL, xxhash32 BIGINT NOT NULL,
    sha256 BLOB(32) NOT NULL, last_scan INTEGER DEFAULT (strftime('%s', 'now'))
);

CREATE TABLE IF NOT EXISTS scan_sessions (id INTEGER PRIMARY KEY, path_hash BIGINT DEFAULT 0,
    scan_path TEXT NOT NULL, created_at BIGINT DEFAULT (strftime('%s', 'now')),
    file_count INT, duplicate_count INT, strategy_flags INT);

CREATE TABLE IF NOT EXISTS action_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT, session_id INT REFERENCES scan_sessions(id),
    file_path TEXT NOT NULL, action_type TEXT NOT NULL, old_value TEXT, new_value TEXT,
    performed_at BIGINT DEFAULT (strftime('%s', 'now'))
);

CREATE INDEX IF NOT EXISTS idx_action_session ON action_history(session_id);
```

---

## Configuration (`%APPDATA%\DupeCheck\settings.json`)

```json
{
    "name_similarity_threshold": 3,
    "hash_tolerance": 1024,
    "max_concurrent_hashers": 7
}
```

---

## Service Commands (CLI)

- `--install-service <path>` — Install as Windows service with scan path
- `--uninstall-service` — Remove service
- `--service` — Run in foreground as service

---

## Project Structure

```
dupecheck/
├── CMakeLists.txt            # Top-level build configuration
│
├── external/                 # External dependencies (bundled, unzip before build)
│   ├── imgui/               # Dear ImGui source + Win32 backend
│   └── sqlite3/             # SQLite amalgamation
│
├── src/
│   ├── main.cpp              # Entry point: CLI args → GUI or service mode
│   │
│   ├── core/                 # Core type definitions
│   │   ├── FileInfo.h        # FileInfo struct + PathUtils namespace
│   │   ├── Strategy.h        # Strategy enum + StrategyConfig
│   │   └── ActionModel.h     # FileType, ActionType, ActionItem, CliCommand, ServiceArgs
│   │
│   ├── hashing/              # Multi-tier hashing engine
│   │   ├── xxhash/           # Local XxHash32 implementation (inline .h + .cpp)
│   │   ├── HashEngine.{h,cpp}  # Single-pass SHA256+XxHash via Bcrypt API
│   │   └── xxhash_wrapper.h  # Thin C++ wrapper around compute_xxhash32()
│   │
│   ├── scanner/              # File enumeration & caching
│   │   ├── CachedScannerService.{h,cpp}  # Primary scanner with incremental updates
│   │   ├── CachedDatabase.h          # Deprecated alias (extends DatabaseManager)
│   │   ├── FileScanner.{h,cpp}       # Composition-based wrapper over CachedScannerService
│   │   └── DatabaseManager.{h,cpp}   # SQLite persistence layer
│   │
│   ├── engine/               # Duplicate detection strategies (inline headers)
│   │   ├── DuplicateEngine.{h,cpp}  # Strategy dispatching & result merging
│   │   ├── ExactMatch.h              SHA256 match
│   │   ├── NameVariant.h             Levenshtein name similarity
│   │   ├── SizeHashSimilar.h         XxHash binning
│   │   ├── ExtensionFamily.h         Extension family mapping (jpg/jpeg, etc.)
│   │   └── FolderCopy.h              Directory tree hashing with relative paths
│   │
│   ├── organization/         # Batch actions on duplicate groups
│   │   ├── OrganizationSvc.{h,cpp}  # Main action orchestration + undo history
│   │   ├── RenameAction.h            Lightweight rename helper (kept for compatibility)
│   │   ├── MoveAction.h              Lightweight move helper
│   │   ├── DeleteAction.h            Lightweight delete helper
│   │   └── SymlinkAction.h           Lightweight symlink creation/undo
│   │
│   ├── service/              # Windows Service + CLI
│   │   ├── ServiceHost.{h,cpp}    Service registration & lifecycle
│   │   └── NamedPipeServer.{h,cpp> GUI-service IPC (named pipe)
│   │
│   ├── gui/                  # ImGui-based user interface (Win32 backend)
│   │   ├── Controls.cpp        Path input, scan/browse buttons
│   │   ├── PreviewPanel.{h,cpp}  Action preview widget per group
│   │   ├── SettingsDialog.{h,cpp>  Modal settings dialog
│   │   └── ImGuiView.{h,cpp}       Main window + event loop (run_gui)
│   │
│   └── utils/                # Shared utilities
│       ├── JsonConfig.{h,cpp>  Lightweight JSON config reader/writer
│       ├── Levenshtein.h      Templated edit-distance algorithm
│       └── ExtensionFamilyMap.h Built-in extension family mappings
│
├── resources/              # Application resources
│   └── appicon.ico         Windows icon resource

## Key Design Decisions

1. **Single-pass hashing**: `HashEngine::compute()` streams through the file once, computing both XxHash32 and SHA256 simultaneously — no double I/O needed.
2. **CachedScannerService** is the primary scanner with incremental updates based on file size + mtime comparison against the SQLite database.
3. **WAL mode**: All SQLite databases use Write-Ahead Logging for concurrent read/write access across GUI and service processes.
4. **No ThreadPool dependency**: Batch hashing uses `std::async` directly (per-file async), with one thread per file rather than a bounded pool.

## License

MIT