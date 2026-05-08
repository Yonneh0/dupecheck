#!/bin/bash
# Download required dependencies for DupeCheck build.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "Downloading dependencies..."

# 1. ImGui (v1.90)
IMGUI_URL="https://github.com/ocornut/imgui/releases/download/v1.90.4/imgui-1.90.4.zip"
if [ ! -d "$SCRIPT_DIR/imgui" ]; then
    echo "  Downloading ImGui..."
    if curl -L "$IMGUI_URL" -o imgui-1.90.4.zip; then
        unzip -q imgui-1.90.4.zip -d "$PROJECT_DIR/external"
        rm -f imgui-1.90.4.zip
    else
        echo "  Error: Failed to download ImGui." >&2
        exit 1
    fi
fi

# 2. SQLite (v3.46)
SQLITE_URL="https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip"
if [ ! -f "$SCRIPT_DIR/sqlite3/sqlite3.c" ]; then
    mkdir -p "$SCRIPT_DIR/sqlite3"
    echo "  Downloading SQLite..."
    if curl -L "$SQLITE_URL" | tar xz -C "$SCRIPT_DIR/sqlite3/" --strip-components=1; then
        : # OK
    else
        echo "  Error: Failed to download/extract SQLite." >&2
        exit 1
    fi
fi

# 3. GoogleTest (for tests)
GTEST_URL="https://github.com/google/googletest/releases/download/v1.14.0/googletest-1.14.0.tar.gz"
if [ ! -f "$SCRIPT_DIR/googletest/CMakeLists.txt" ]; then
    echo "  Downloading GoogleTest..."
    if curl -L "$GTEST_URL" -o googletest-1.14.0.tar.gz && \
       tar xzf googletest-1.14.0.tar.gz -C "$SCRIPT_DIR/" --strip-components=1; then
        rm -f googletest-1.14.0.tar.gz
    else
        echo "  Error: Failed to download/extract GoogleTest." >&2
        exit 1
    fi
fi

echo "Dependencies downloaded successfully."
