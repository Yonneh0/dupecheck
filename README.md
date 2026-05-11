# DupeCheck — Duplicate File Finder & Organizer

A fast duplicate file finder for Windows built with C++20. Scans folders and drives using multi-tier hash analysis (XxHash32 + SHA256) to detect exact duplicates, renamed copies, modified files, extension variants, and folder-level copies in a single pass.

## Features

- **Multi-strategy duplicate detection:**
  - **Exact Match (1)** — identical SHA256 hash (exact content copy)
  - **Name Variants (2)** — same content but different names within Levenshtein edit distance threshold
  - **Size+Hash Similar (4)** — similar size + similar XxHash32, indicating likely modified copies
  - **Extension Family (8)** — same content across extension families (`jpg`/`jpeg`, `docx`/`doc`)
  - **Folder Copy (16)** — entire directory trees copied to new locations using hierarchical tree hashing

- **Cached incremental scanning** via a SQLite database in WAL mode. Only re-hashes files that have changed since the last scan.

- **Batch organization actions:** rename with suffixes, move to duplicate folders, delete copies, create symlinks, or archive duplicates. Full undo support.

- **High-performance parallel hashing** — computes XxHash32 and SHA256 in a single I/O pass using multi-threaded workers.

- **Windows Service** — installable service that periodically scans your configured path. CLI commands for installation (`--install-service`, `--uninstall-service`).

## Building

### Prerequisites
- CMake 3.24+
- MSVC (Visual Studio 2019+) with Windows SDK
- External dependencies: ImGui, SQLite bundled in the `external/` directory. Tests compile as standalone executables without GoogleTest.

### Quick Start
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
.\build\Release\DupeCheck.exe
```

## Usage

1. Open DupeCheck and enter a folder or drive path (or click Browse).
2. Click **Scan** — results appear categorized by duplicate type.
3. Use **Apply All** / **Undo Last** to manage actions per group.
4. Open **Settings** to configure detection thresholds, extension families, and service options.

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
├── external/                       # External dependencies (ImGui, SQLite)
│   ├── imgui/                      # Dear ImGui source files + Win32 backend
│   └── sqlite3/                    # SQLite amalgamation
├── src/
│   ├── main.cpp                    # Entry point: WinMain (GUI), main (service)
│   ├── cli.cpp                     # CLI argument parsing (--install-service, etc.)
│   │
│   ├── core/                       # Core type definitions
│   │   ├── FileInfo.h              # File metadata struct + PathUtils namespace
│   │   ├── Strategy.h              # Strategy enum + config
│   │   └── ActionModel.h           # Actions (rename, move, delete, etc.)
│   │
│   ├── hashing/                    # Multi-tier hashing engine
│   │   ├── xxhash/                 # Local XxHash32 implementation
│   │   │   ├── xxhash.cpp/.h       # Core algorithm + streaming API
│   │   └── HashEngine.{cpp,h}      # Single-pass SHA256+XxHash, thread pool
│   │   └── ThreadPool.cpp          # Thread pool with work queue
│   │
│   ├── scanner/                    # File enumeration & caching
│   │   ├── FileScanner.{cpp,h}     # Recursive directory traversal + hashing
│   │   └── CachedScannerService.{cpp,h}  # SQLite-backed incremental scanning
│   │
│   ├── engine/                     # Duplicate detection strategies
│   │   ├── DuplicateEngine.{cpp,h} # Strategy dispatching & result merging
│   │   ├── ExactMatch.h            # SHA256 match
│   │   ├── NameVariant.h           # Levenshtein name similarity
│   │   ├── SizeHashSimilar.h       # Size + XxHash binning
│   │   ├── ExtensionFamily.h       # Extension family mapping
│   │   └── FolderCopy.h            # Directory tree hashing
│   │
│   ├── organization/               # Batch actions on duplicate groups
│   │   ├── OrganizationSvc.{cpp,h} # Action orchestration & history
│   │   ├── RenameAction.h          # Rename with configurable format strings
│   │   ├── MoveAction.h            # Move to duplicates folder
│   │   ├── DeleteAction.h          # Delete copies from disk
│   │   ├── SymlinkAction.h         # CreateWindows symbolic links
│   │   ├── ArchiveAction.h         # Zip/archive duplicates
│   │   └── UndoManager.h           # History stack for undo support
│   │
│   ├── database/                   # SQLite persistence layer
│   │   ├── DatabaseManager.{cpp,h} # Schema, CRUD, WAL mode
│   │
│   ├── service/                    # Windows Service + IPC
│   │   ├── ServiceHost.{cpp,h}     # Service registration & lifecycle
│   │   └── NamedPipeServer.{cpp,h} # IPC pipe for GUI ↔ service communication
│   │
│   ├── gui/                        # ImGui-based user interface (Win32 backend)
│   │   ├── ImGuiView.{cpp,h}       # Main window rendering loop
│   │   ├── Controls.cpp            # Path input, scan/browse buttons
│   │   ├── PreviewPanel.{cpp,h}    # Action preview widget per group
│   │   └── SettingsDialog.{cpp,h}  # Modal settings dialog
│   │
│   └── utils/                      # Shared utilities
│       ├── JsonConfig.{cpp,h}      # Lightweight JSON config reader/writer (UTF-8)
│       ├── Levenshtein.h           # Templated Levenshtein distance algorithm
│       └── ExtensionFamilyMap.h    # Built-in extension family mappings
│
├── resources/                      # Application resources
│   └── dupecheck.rc                # Windows resource file (icon)
│
└── tests/                          # Unit tests (standalone, no GoogleTest)
```

## License

MIT