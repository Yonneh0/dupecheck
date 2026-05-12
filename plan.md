# DupeCheck — Development Plan

## Overview

DupeCheck is a C++20 Windows application that finds duplicate files across folders/drives using multi-tier hashing (XxHash32 + SHA256) and provides batch organization actions. Runs as both GUI (.exe) and installable Windows Service with SQLite-backed caching.

---

## Technology Stack

| Layer | Choice |
|-------|--------|
| Language | C++20, C17 |
| Build System | CMake 3.24+ |
| GUI | ImGui + Dear ImGui Win32 backend |
| Hashing | Local XxHash32 (`src/hashing/xxhash.c`), Windows native SHA256 via Bcrypt API |
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
│  │  │ FileScanner│ │ CachedScannerSvc   │    │ │
│  │  │ (enumerate,│ │ (SQLite cache)      │    │ │
│  │  │  XxHash32) │ └────────┬───────────┘    │ │
│  │  └────┬───────┘          │                │ │
│  │  ┌────▼──────────────────▼──────────┐     │ │
│  │  │   Duplicate Engine               │      │ │
│  │  │ (strategies, matching, grouping) │      │ │
│  │  └────┬─────────────────────┬──────┘       │ │
│  │       │                     │              │ │
│  │  ┌────▼────────┐    ┌──────▼───────────┐   │ │
│  │  │ HashEngine  │    │ OrganizationSvc   │   │ │
│  │  │ (SHA256,    │    │ (rename/move/     │   │ │
│  │  │  XxHash32)  │    │ delete/symlink)   │   │ │
│  │  └─────────────┘    └──────────────────┘   │ │
│  └─────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────┘

SQLite DB: %APPDATA%\DupeCheck\dupecheck.db (WAL mode)
```

---

## Core Data Model

```cpp
struct FileInfo {
    std::wstring path;      // Full file path
    uint64_t size;          // Bytes
    long long mtime;        // Seconds since epoch
    XxHash32 xxhash;        // Tier-1 hash
    Sha256 sha256;          // Tier-2 hash
};

enum class Strategy : uint32_t {
    ExactMatch = 1,         // SHA256 match
    NameVariant = 2,        // Same content + name within Levenshtein distance
    SizeHashSimilar = 4,    // Similar size + XxHash in same bin
    ExtensionFamily = 8,    // Same content across extension family
    FolderCopy = 16,        // Entire directory trees copied
};

enum class ActionType { Rename, MoveToDuplicatesFolder, Delete, CreateSymlink, Archive };
```

---

## Detection Strategies

| Strategy | Value | Description |
|----------|-------|-------------|
| Exact Match | 1 | Group by SHA256 hash |
| Name Variant | 2 | Same content + name within Levenshtein distance (default: 3) |
| Size+Hash Similar | 4 | Binned XxHash32 comparison (tolerance: 1024 bytes) |
| Extension Family | 8 | Same content across extension families (`jpg`/`jpeg`) |
| Folder Copy | 16 | Directory-level tree hashing via SHA256 |

---

## Database Schema

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

## Service Commands

- `--install-service <path>` — Install as Windows service with scan path
- `--uninstall-service` — Remove service
- `--service` — Run in foreground as service

---

## Project Structure

```
dupecheck/
├── CMakeLists.txt            # Top-level build configuration
├── README.md                 # User-facing documentation
│
├── external/                 # External dependencies (bundled)
│   ├── imgui/               # Dear ImGui source + Win32 backend
│   └── sqlite3/             # SQLite amalgamation
│
├── src/
│   ├── main.cpp             # Entry point: CLI args → GUI or service
│   ├── cli.cpp              # Command-line argument parsing
│   │
│   ├── core/                # Core types
│   │   ├── ActionModel.h    # FileType, ActionType, ActionItem, ActionHistoryEntry
│   │   ├── FileInfo.h       # FileInfo struct + PathUtils namespace
│   │   └── Strategy.h       # Strategy enum + StrategyConfig
│   │
│   ├── hashing/             # Multi-tier hashing engine
│   │   ├── xxhash/          # Local XxHash32 implementation
│   │   ├── HashEngine.{h,cpp}  # Single-pass SHA256+XxHash, batch compute
│   │   └── ThreadPool.cpp   # Thread pool for parallel hashing
│   │
│   ├── scanner/             # File enumeration & caching
│   │   ├── FileScanner.{h,cpp}     # Recursive traversal + single-file hashing
│   │   └── CachedScannerService.{h,cpp}  # SQLite-backed incremental scanning
│   │
│   ├── engine/              # Duplicate detection strategies (one per file)
│   │   ├── DuplicateEngine.{h,cpp}  # Strategy dispatching & result merging
│   │   ├── ExactMatch.h     SHA256 match
│   │   ├── NameVariant.h    Levenshtein name similarity
│   │   ├── SizeHashSimilar.h XxHash binning
│   │   ├── ExtensionFamily.h  Extension family mapping
│   │   └── FolderCopy.h     Directory tree hashing
│   │
│   ├── organization/        # Batch actions on duplicate groups
│   │   ├── OrganizationSvc.{h,cpp}  # Main action orchestration (rename, move, delete)
│   │   ├── RenameAction.h       Lightweight rename helper
│   │   ├── MoveAction.h         Lightweight move helper
│   │   ├── DeleteAction.h       Lightweight delete helper
│   │   ├── SymlinkAction.h      Symlink creation/undo
│   │   ├── ArchiveAction.h      Archive/zip duplicates
│   │   └── UndoManager.h        History stack for undo support
│   │
│   ├── database/          # SQLite persistence layer
│   │   └── DatabaseManager.{h,cpp}  Schema, CRUD operations, WAL mode
│   │
│   ├── service/           # Windows Service + IPC
│   │   ├── ServiceHost.{h,cpp}  Service registration & lifecycle
│   │   └── NamedPipeServer.{h,cpp}  IPC pipe for GUI ↔ service
│   │
│   ├── gui/               # ImGui-based user interface (Win32 backend)
│   │   ├── Controls.cpp     Path input, scan/browse buttons
│   │   ├── PreviewPanel.{h,cpp}  Action preview widget per group
│   │   └── SettingsDialog.{h,cpp}  Modal settings dialog
│   │
│   └── utils/             # Shared utilities
│       ├── JsonConfig.{h,cpp}     Lightweight JSON config reader/writer
│       ├── Levenshtein.h          Templated edit-distance algorithm
│       └── ExtensionFamilyMap.h   Built-in extension family mappings
│
├── resources/             # Application resources
│   └── appicon.ico        Windows icon resource
│
└── tests/                 # Unit tests (standalone)
```

---

## Completed Phases

Foundation ✅ | Hashing Engine ✅ (single-pass streaming API) | Scanner & Caching ✅ (WAL mode, upsert/delete) | Duplicate Detection ✅ (all five strategies as inline headers) | Organization Actions ✅ | GUI ✅ (Win32 + ImGui) | Service ✅ (simplified host with named pipe IPC)