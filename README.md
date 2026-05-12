# DupeCheck — Duplicate File Finder & Organizer

A fast duplicate file finder for Windows built with C++20. Scans folders and drives using multi-tier hash analysis (XxHash32 + SHA256) to detect exact duplicates, renamed copies, modified files, extension variants, and folder-level copies in a single pass.

## Features

- **Multi-strategy duplicate detection:**
  - **Exact Match** — identical SHA256 hash (exact content copy)
  - **Name Variants** — same content but different names within Levenshtein distance threshold
  - **Size+Hash Similar** — similar size + XxHash32 in the same bin, indicating likely modified copies
  - **Extension Family** — same content across extension families (`jpg`/`jpeg`, `docx`/`doc`, etc.)
  - **Folder Copy** — entire directory trees copied to new locations using hierarchical tree hashing

- **Cached incremental scanning** via a SQLite database at `%APPDATA%\DupeCheck\dupecheck.db`. Only re-hashes files that have changed since the last scan, based on size and modification time.

- **Batch organization actions:** rename with suffixes, move to duplicate folders, delete copies, create symlinks, or archive duplicates. Full undo support via an action history stack.

- **Single-pass hashing** — computes XxHash32 and SHA256 in a single I/O pass using `std::async` across multiple threads (one per file).

## Building

### Prerequisites
- CMake 3.24+
- MSVC (Visual Studio 2019+) with Windows SDK
- External dependencies: unzip the bundled archives into their respective directories before building.

```bash
# Extract external dependencies
tar -xf external/imgui-1.92.7.zip -C external/
unzip external/sqlite-amalgamation-3460100.zip -d external/

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

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
4. Open **Settings** to configure detection thresholds.

### Windows Service Installation
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

See [plan.md](plan.md) for the full architecture diagram and design decisions.

## License

MIT