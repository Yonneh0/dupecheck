# DupeCheck — Duplicate File Finder & Organizer

A fast duplicate file finder for Windows built with C++20. Scans folders and drives using multi-tier hash analysis (XxHash32 + SHA256) to detect exact duplicates, renamed copies, modified files, extension variants, and folder-level copies in a single I/O pass.

## Features

- **Multi-strategy duplicate detection:**
  - **Exact Match** — identical SHA256 hash (exact content copy)
  - **Name Variants** — same content but different names within Levenshtein distance threshold
  - **Size+Hash Similar** — similar size + XxHash32 in the same bin, indicating likely modified copies
  - **Extension Family** — same content across extension families (`jpg`/`jpeg`, `docx`/`doc`, etc.)
  - **Folder Copy** — entire directory trees copied to new locations using hierarchical tree hashing

- **Cached incremental scanning** via a SQLite database at `%APPDATA%\DupeCheck\dupecheck.db`. Only re-hashes files that have changed since the last scan, based on size and modification time.

- **Batch organization actions:** rename with suffixes, move to duplicate folders, delete copies, create symlinks, or archive duplicates. Full undo support via an action history stack.

- **Single-pass hashing** — computes XxHash32 and SHA256 in a single I/O pass using `std::async` (one thread per file) across multiple threads.

## Building

### Prerequisites
- CMake 3.24+
- MSVC (Visual Studio 2019+) with Windows SDK

### Setup external dependencies
Extract the bundled archives into their respective directories before building:

```bash
# Extract imgui (contains ~35 source files)
tar -xf external/imgui-1.92.7.zip -C external/
unzip external/sqlite-amalgamation-3460100.zip -d external/
```

After extraction the directory structure should look like:
```
external/
├── imgui/              ← contains *.cpp and backends/imgui_impl_win32.cpp
└── sqlite3/            ← contains sqlite3.c, sqlite3.h, sqlite3ext.h
```

### Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Quick start
```bash
.\build\DupeCheck.exe
```

## Usage

1. Open DupeCheck and enter a folder or drive path (or click **Browse**).
2. Click **Scan** — results appear categorized by duplicate type, with the original file marked in green (`O`) and duplicates in yellow (`D`).
3. Use **Apply All** / **Undo Last** to manage actions per group, or **Undo All Actions** at the bottom.
4. Open **Settings** to configure detection thresholds — settings are saved to `%APPDATA%\DupeCheck\settings.json` and loaded automatically on dialog open.

### Windows Service installation
```bash
DupeCheck.exe --install-service C:\Your\Scan\Path
DupeCheck.exe --uninstall-service
DupeCheck.exe --service              # Run in foreground as service
```

## Configuration

The settings file is stored at `%APPDATA%\DupeCheck\settings.json`:

| Setting              | Default | Description                                          |
|----------------------|---------|------------------------------------------------------|
| Name Similarity Threshold | 3   | Levenshtein distance for name-variant detection      |
| Hash Tolerance       | 1024    | XxHash32 bin size (bytes) for similarity grouping    |
| Max Concurrent Hashers | Auto  | Number of parallel hash workers (= CPU cores - 1)   |

## Architecture

DupeCheck uses a layered architecture with single-pass hashing:

1. **Tier 1 (XxHash32):** Fast non-cryptographic hash computed during file enumeration for initial grouping of candidate duplicates (~50 GB/s per core).
2. **Tier 2 (SHA256):** Full cryptographic hash via Windows Bcrypt API, computed in the same I/O pass to confirm exact matches within XxHash groups and across extension families.

The `CachedScannerService` maintains a SQLite database at `%APPDATA%\DupeCheck\dupecheck.db`. On subsequent scans it compares file size and modification time to identify unchanged files, re-hashing only modified ones.

## Project Structure

```
src/
├── main.cpp              # Entry point — CLI args → GUI or service mode
│
├── core/                 # Core type definitions
│   ├── FileInfo.h        # FileInfo struct + PathUtils namespace (path helpers)
│   ├── Strategy.h        # Strategy enum + StrategyConfig
│   └── ActionModel.h     # FileType, ActionType, ActionItem, CliCommand
│
├── engine/               # Header-only detection strategies (one function per file)
│   ├── DuplicateEngine.{h,cpp}  # Strategy dispatching & result merging
│   ├── ExactMatch.h              # SHA256 exact-match grouping
│   ├── NameVariant.h             # Levenshtein name similarity
│   ├── SizeHashSimilar.h         # XxHash binning for similar files
│   ├── ExtensionFamily.h         # Cross-extension family detection (jpg/jpeg)
│   └── FolderCopy.h              # Directory tree hashing for folder copies
│
├── scanner/
│   └── CachedScannerService.{h,cpp}  # Incremental caching over SQLite
│
├── hashing/
│   ├── HashEngine.{h,cpp}    # SHA256 (BCrypt) + XxHash32 single-pass computation
│   ├── xxhash/               # Local XXH32 implementation
│   │   ├── xxhash.h          # XXH32 public API + streaming interface
│   │   └── xxhash.cpp        # Compilation unit for CMake linking
│   └── xxhash_wrapper.h      # Thin C++ wrapper around compute_xxhash32()
│
├── database/
│   └── DatabaseManager.{h,cpp}  # SQLite persistence layer (WAL mode)
│
├── organization/         # Batch actions on duplicate groups
│   ├── OrganizationSvc.{h,cpp}  # Main action orchestration + undo history
│   └── MergeAction.h            # Move, Archive, Delete, Symlink helpers
│
├── service/              # Windows Service + CLI
│   ├── ServiceHost.{h,cpp}      # Service registration & lifecycle
│   └── NamedPipeServer.{h,cpp>  # GUI-service IPC (named pipe)
│
├── gui/                  # ImGui-based user interface (Win32 backend)
│   ├── Controls.cpp/h            # Path input, scan/browse buttons + status indicators
│   ├── PreviewPanel.{h,cpp}     # Action preview widget per duplicate group
│   ├── SettingsDialog.{h,cpp>   # Modal settings dialog
│   └── ImGuiView.{h,cpp}         Main window + event loop (run_gui)
│
└── utils/                # Shared utilities
    ├── JsonConfig.{h,cpp}        Lightweight JSON config reader/writer
    ├── Levenshtein.h             Templated edit-distance algorithm
    └── ExtensionFamilyMap.h      Built-in extension family mappings
```

## License

MIT