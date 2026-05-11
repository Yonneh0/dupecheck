# DupeCheck — Development Plan

## Overview

DupeCheck is a C++20 Windows application that finds duplicate files across folders/drives using multi-tier hashing (XxHash32 + SHA256) and provides batch organization actions. Runs as both GUI (.exe) and installable Windows Service with SQLite-backed caching.

---

## Technology Stack

| Layer | Choice |
|-------|--------|
| Language | C++20 |
| Build System | CMake 3.24+ |
| GUI | ImGui + Dear ImGui Win32 backend |
| Hashing | Local XxHash32 (`src/hashing/xxhash.c`), Windows native SHA256 via Bcrypt API |
| Database | sqlite3.h with WAL mode for multi-process concurrency |
| Service | Native Windows Service API |

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
    id INTEGER PRIMARY KEY, path TEXT UNIQUE NOT NULL COLLATE NOCASE,
    size BIGINT NOT NULL, mtime BIGINT NOT NULL, xxhash32 BIGINT NOT NULL,
    sha256 BLOB(32) NOT NULL, last_scan INTEGER DEFAULT (strftime('%s', 'now'))
);

CREATE TABLE IF NOT EXISTS scan_sessions (id INTEGER PRIMARY KEY, path_hash BIGINT,
    scan_path TEXT NOT NULL, created_at BIGINT, file_count INT, duplicate_count INT, strategy_flags INT);

CREATE TABLE IF NOT EXISTS action_history (id INTEGER PRIMARY KEY, session_id INT REFERENCES scan_sessions(id),
    file_path TEXT NOT NULL, action_type TEXT NOT NULL, old_value TEXT, new_value TEXT, performed_at BIGINT DEFAULT (strftime('%s', 'now')));
```

---

## Configuration (`%APPDATA%\DupeCheck\settings.json`)

```json
{
    "name_similarity_threshold": 3,
    "hash_tolerance": 1024,
    "max_concurrent_hashers": 7,
    "service_enabled": true
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
├── CMakeLists.txt
├── README.md
├── external/{imgui/, sqlite3/}
└── src/
    ├── main.cpp, cli.cpp
    ├── core/{FileInfo.h, Strategy.h, ActionModel.h}
    ├── hashing/{HashEngine.{cpp,h}, ThreadPool.cpp, xxhash_wrapper.h}
    ├── scanner/{FileScanner.{cpp,h}, CachedScannerService.{cpp,h}}
    ├── engine/{DuplicateEngine.{cpp,h}, ExactMatch.h, NameVariant.h, SizeHashSimilar.h, ExtensionFamily.h, FolderCopy.h}
    ├── organization/{OrganizationSvc.{cpp,h}, RenameAction.h, MoveAction.h, DeleteAction.h, SymlinkAction.h, ArchiveAction.h, UndoManager.h}
    ├── database/{DatabaseManager.{cpp,h}}
    ├── service/{ServiceHost.{cpp,h}, NamedPipeServer.{cpp,h}}
    ├── gui/{ImGuiView.{cpp,h}, Controls.cpp, PreviewPanel.{cpp,h}, SettingsDialog.{cpp,h}}
    └── utils/{JsonConfig.{cpp,h}, Levenshtein.h, ExtensionFamilyMap.h}
```

---

## Completed Phases

Foundation ✅ | Hashing Engine ✅ (corrected streaming API) | Scanner & Caching ✅ (fixed upsert/delete) | Duplicate Detection ✅ (all five strategies) | Organization Actions ✅ | GUI ✅ (Win32 backend) | Service ✅ (simplified host)