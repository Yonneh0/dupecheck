# DupeCheck — Duplicate File Finder & Organizer

A fast duplicate file finder for Windows built with C++20. Scans folders and drives using multi-tier hash analysis (XxHash32 + SHA256) to detect exact duplicates, renamed copies, modified files, extension variants, and folder-level copies in a single pass.

## Features

- **Multi-strategy duplicate detection:**
  - **Exact Match (1)** — identical SHA256 hash (exact content copy)
  - **Name Variants (2)** — same content but different names within Levenshtein edit distance threshold
  - **Size+Hash Similar (4)** — similar size + similar XxHash32, indicating likely modified copies
  - **Extension Family (8)** — same content across extension families (`jpg`/`jpeg`, `docx`/`doc`)
  - **Folder Copy (16)** — entire directory trees copied to new locations using hierarchical tree hashing

- **Cached incremental scanning** via a SQLite database in WAL mode. Only re-hashes files that have changed since the last scan, based on size and modification time.

- **Batch organization actions:** rename with suffixes, move to duplicate folders, delete copies, create symlinks, or archive duplicates. Full undo support.

- **High-performance parallel hashing** — computes XxHash32 and SHA256 in a single I/O pass using multi-threaded workers (ThreadPool).

## Building

### Prerequisites
- CMake 3.24+
- MSVC (Visual Studio 2019+) with Windows SDK
- External dependencies: unzip ImGui and SQLite into `external/imgui/` and `external/sqlite3/`.

### Quick Start
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
.\build\DupeCheck.exe
```

## Usage

1. Open DupeCheck and enter a folder or drive path (or click Browse).
2. Click **Scan** — results appear categorized by duplicate type.
3. Use **Apply All** / **Undo Last** to manage actions per group.
4. Open **Settings** to configure detection thresholds and extension families.

### Windows Service Installation
```bash
DupeCheck.exe --install-service C:\Your\Scan\Path
DupeCheck.exe --uninstall-service
```

## Architecture

DupeCheck uses a layered architecture with two-tier hashing:

1. **Tier 1 (XxHash32):** Fast non-cryptographic hash computed during file enumeration for initial grouping of candidate duplicates (~50 GB/s per core).
2. **Tier 2 (SHA256):** Full cryptographic hash via Bcrypt API, computed in the same I/O pass to confirm exact matches within XxHash groups and across extension families.

The `CachedScannerService` maintains a SQLite database at `%APPDATA%\DupeCheck\dupecheck.db`. On subsequent scans it compares file size and modification time to identify unchanged files, re-hashing only modified ones.

## Configuration

The settings file is stored at `%APPDATA%\DupeCheck\settings.json`:

| Setting              | Default | Description                                          |
|----------------------|---------|------------------------------------------------------|
| Name Similarity Threshold | 3   | Levenshtein distance for name-variant detection      |
| Hash Tolerance       | 1024    | XxHash32 bin size (bytes) for size+hash similarity grouping |
| Max Concurrent Hashers | Auto  | Number of parallel hash workers (= CPU cores - 1)   |

## Project Structure

```
dupecheck/
├── CMakeLists.txt                  # Top-level build configuration
├── README.md                       # This file
├── external/                       # External dependencies (ImGui, SQLite; unzip before build)
│   ├── imgui/                      # Dear ImGui source files + Win32 backend
│   └── sqlite3/                    # SQLite amalgamation
├── src/
│   ├── main.cpp                    # Entry point: CLI args → GUI or service
│   ├── cli.cpp                     # CLI argument parsing (--install-service, etc.)
│   │
│   ├── core/                       # Core type definitions
│   │   ├── ActionModel.h           # FileType, ActionType, ActionItem, ActionHistoryEntry
│   │   ├── FileInfo.h              # FileInfo struct + PathUtils namespace (enumerate_files)
│   │   └── Strategy.h              # Strategy enum + StrategyConfig
│   │
│   ├── hashing/                    # Multi-tier hashing engine
│   │   ├── xxhash/                 # Local XxHash32 implementation
│   │   ├── HashEngine.{h,cpp}      # Single-pass SHA256+XxHash, batch compute (ThreadPool)
│   │   └── ThreadPool.cpp          # Thread pool with work queue
│   │
│   ├── scanner/                    # File enumeration & caching
│   │   ├── CachedDatabase.{h,cpp}  # SQLite cache layer (shared by both scanners)
│   │   ├── CachedScannerService.{h,cpp}  # Primary scanner with incremental updates
│   │   └── FileScanner.{h,cpp}     # Legacy scanner — delegates to CachedDatabase
│   │
│   ├── engine/                     # Duplicate detection strategies (inline headers)
│   │   ├── DuplicateEngine.{h,cpp} # Strategy dispatching & result merging
│   │   ├── ExactMatch.h            SHA256 match
│   │   ├── NameVariant.h           Levenshtein name similarity
│   │   ├── SizeHashSimilar.h       Size + XxHash binning
│   │   ├── ExtensionFamily.h       Extension family mapping
│   │   └── FolderCopy.h            Directory tree hashing (compute_tree_hash)
│   │
│   ├── organization/               # Batch actions on duplicate groups
│   │   ├── OrganizationSvc.{h,cpp} # Main action orchestration (rename, move, delete)
│   │   ├── RenameAction.h          Lightweight rename helper
│   │   ├── MoveAction.h            Lightweight move helper
│   │   ├── DeleteAction.h          Lightweight delete helper
│   │   ├── SymlinkAction.h         Lightweight symlink creation/undo
│   │   ├── ArchiveAction.h         Lightweight archive/zip duplicates
│   │   └── UndoManager.h           History stack for undo support
│   │
│   ├── database/                   # SQLite persistence layer
│   │   └── DatabaseManager.{h,cpp} # Schema, CRUD operations, WAL mode
│   │
│   ├── service/                    # Windows Service + IPC
│   │   ├── ServiceHost.{h,cpp}     Service registration & lifecycle
│   │   └── NamedPipeServer.{h,cpp} # IPC pipe for GUI ↔ service communication
│   │
│   ├── gui/                        # ImGui-based user interface (Win32 backend)
│   │   ├── Controls.cpp            Path input, scan/browse buttons
│   │   ├── PreviewPanel.{h,cpp}    Action preview widget per group
│   │   └── SettingsDialog.{h,cpp}  Modal settings dialog
│   │
│   └── utils/                      # Shared utilities
│       ├── JsonConfig.{h,cpp}      Lightweight JSON config reader/writer
│       ├── Levenshtein.h           Templated Levenshtein distance algorithm
│       └── ExtensionFamilyMap.h    Built-in extension family mappings
│
├── resources/                      # Application resources
│   └── appicon.ico                 Windows icon resource

## License

MIT