# DupeCheck — Duplicate File Finder & Organizer

A fast, intelligent duplicate file finder for Windows built with C++20. Scans folders and drives using multi-tier hash analysis (XxHash32 + SHA256) to detect exact duplicates, renamed copies, modified files, extension variants, and folder-level copies in a single pass.

## Features

- **Multi-strategy duplicate detection:**
  - **Exact Match (1)** — identical SHA256 hash (exact content copy)
  - **Name Variants (2)** — same content but different names within Levenshtein edit distance threshold
  - **Size+Hash Similar (4)** — similar size + similar XxHash32, indicating likely modified copies
  - **Extension Family (8)** — same content across extension families (`jpg`/`jpeg`, `docx`/`doc`)
  - **Folder Copy (16)** — entire directory trees copied to new locations using hierarchical tree hashing

- **Cached incremental scanning** — only re-hashes files that have changed since the last scan (using size and mtime comparison), via a SQLite database in WAL mode. Designed for large drives.

- **Batch organization actions:** rename with suffixes/prefixes/numbers, move to duplicate folders, delete copies, create symlinks, or archive duplicates. Full undo support.

- **High-performance parallel hashing** — computes XxHash32 and SHA256 in a single I/O pass using multi-threaded workers.

- **Windows Service** — installable service that periodically scans your configured path and maintains the SQLite database. CLI commands for installation.
- **Named Pipe IPC** — communicates with GUI clients via `\\.\pipe\dupecheck` pipe.

## Building

### Prerequisites

- CMake 3.24+
- MSVC (Visual Studio 2019 or later) with Windows SDK
- For tests: GoogleTest (downloaded automatically by FetchContent if not present)

### Quick Start

```bash
# Download dependencies (optional — they can also be added via FetchContent/submodules)
.\external\download_dependencies.sh

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Run the GUI application
.\build\Release\DupeCheck.exe
```

### Service Build Options

- `BUILD_TESTS` (default: ON) — Compile unit tests
- `BUILD_SERVICE` (default: ON) — Build Windows service binary (`DupeCheckService.exe`)

## Usage

1. Open DupeCheck and enter a folder or drive path (or click the folder icon).
2. Click **Scan** — results appear categorized by duplicate type.
3. Click **Preview** to see proposed batch actions for each duplicate group.
4. Select or deselect individual actions, then click **Apply**.
5. Use **Undo** to reverse applied actions.
6. Open **Settings** to configure detection thresholds (name similarity, hash tolerance), extension families, and service options.

### Windows Service Installation

```bash
# Install the service with a specific scan path.
DupeCheck.exe --install-service C:\Your\Scan\Path

# Uninstall the service
DupeCheck.exe --uninstall-service
```

The service installs itself to `%APPDATA%\DupeCheck\dupecheck.db` and runs as a Windows service, maintaining its own SQLite database with incremental scans.

## Architecture

DupeCheck uses an MVVM pattern with two-tier hashing:

1. **Tier 1 (XxHash32):** Fast non-cryptographic hash computed during file enumeration for initial grouping of candidate duplicates (~50 GB/s per core).
2. **Tier 2 (SHA256):** Full cryptographic hash computed in the same I/O pass to confirm exact matches within XxHash groups and across extension families.

The `CachedScannerService` maintains a SQLite database at `%APPDATA%\DupeCheck\dupecheck.db`. On subsequent scans it compares file size and modification time to identify unchanged files, re-hashing only modified ones. Deleted files are automatically removed from the cache via a clean delete-all / re-insert cycle in WAL mode.

The service runs as a separate process (or registered Windows service) that:
- Periodically rescans its configured path using incremental hashing
- Listens for SCAN/GET_RESULTS commands on `\\.\pipe\dupecheck`
- Shares the SQLite database with GUI clients via WAL mode

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
├── external/                       # External dependencies (downloaded or bundled)
│   ├── imgui/                      # Dear ImGui source files
│   └── sqlite3/                    # SQLite amalgamation
├── src/
│   ├── main.cpp                    # Entry points: WinMain (GUI) and main (service)
│   ├── cli.cpp                     # CLI argument parsing (--install-service, etc.)
│   │
│   ├── core/                       # Core type definitions
│   │   ├── FileInfo.h              # File metadata struct + PathUtils namespace
│   │   ├── Strategy.h              # Strategy enum + config
│   │   └── ActionModel.h           # Actions (rename, move, delete, etc.)
│   │
│   ├── hashing/                    # Multi-tier hashing engine
│   │   ├── xxhash/                 # Local XxHash32 implementation
│   │   │   ├── xxhash.c            # Core XxHash algorithm + streaming API
│   │   │   └── xxhash.h            # Header definitions
│   │   ├── HashEngine.cpp/.h       # Single-pass SHA256+XxHash, thread pool
│   │   └── xxhash_wrapper.h        # Convenience wrapper (C++ only)
│   │
│   ├── scanner/                    # File enumeration & caching
│   │   ├── FileScanner.cpp/.h      # Recursive directory traversal + hashing
│   │   └── CachedScannerService.cpp/.h  # SQLite-backed incremental scanning
│   │
│   ├── engine/                     # Duplicate detection strategies
│   │   ├── DuplicateEngine.cpp/.h  # Strategy dispatching & result merging
│   │   ├── ExactMatch.h            # Strategy: SHA256 match
│   │   ├── NameVariant.h           # Strategy: Levenshtein name similarity
│   │   ├── SizeHashSimilar.h       # Strategy: size + XxHash binning
│   │   ├── ExtensionFamily.h       # Strategy: extension family mapping
│   │   └── FolderCopy.h            # Strategy: directory tree hashing (merged duplicates)
│   │
│   ├── organization/               # Batch actions on duplicate groups
│   │   ├── OrganizationSvc.cpp/.h  # Action orchestration & history
│   │   ├── RenameAction.h          # Rename with configurable format strings
│   │   ├── MoveAction.h            # Move to duplicates folder
│   │   ├── DeleteAction.h          # Delete copies from disk
│   │   ├── SymlinkAction.h         # Create Windows symbolic links
│   │   ├── ArchiveAction.h         # Zip/archive duplicates
│   │   └── UndoManager.h           # History stack for undo support
│   │
│   ├── database/                   # SQLite persistence layer
│   │   ├── DatabaseManager.cpp/.h  # Schema, CRUD, WAL mode
│   │
│   ├── service/                    # Windows Service + IPC
│   │   ├── ServiceHost.cpp/.h      # Service registration & lifecycle
│   │   └── NamedPipeServer.cpp/.h  # IPC pipe for GUI ↔ service communication
│   │
│   ├── gui/                        # ImGui-based user interface
│   │   ├── ImGuiView.cpp/.h        # Main window rendering loop (GDI backend)
│   │   ├── Controls.cpp            # Path input, scan/browse buttons, progress bar
│   │   ├── PreviewPanel.cpp/.h     # Action preview widget per group
│   │   └── SettingsDialog.cpp      # Modal settings dialog (thresholds, extensions)
│   │
│   └── utils/                      # Shared utilities
│       ├── JsonConfig.cpp/.h        # Lightweight JSON config reader/writer (UTF-8)
│       ├── Levenshtein.h           # Levenshtein distance algorithm
│       └── ExtensionFamilyMap.h    # Built-in extension family mappings (cached)
│
├── resources/                      # Application resources
│   └── dupecheck.rc                # Windows resource file (icons, menus)
│
└── tests/                          # Unit tests (GoogleTest)
    ├── test_levenshtein.cpp        # Levenshtein distance tests
    ├── test_hash_engine.cpp        # XxHash32 + SHA256 correctness
    └── test_duplicate_engine.cpp   # Strategy detection unit tests
```

## Resources

A placeholder icon is referenced by `resources/appicon.ico`. If the file exists, CMake copies it to the build output. For a production release, replace with your own `.ico` file or update the resource script accordingly.

## License

MIT