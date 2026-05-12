#pragma once
#include <string>
#include <vector>
#include "../core/FileInfo.h"
#include "../database/DatabaseManager.h"

// Deprecated — kept for backward compatibility. Simply aliases DatabaseManager directly.
class CachedDatabase : public DatabaseManager {
public:
    explicit CachedDatabase(const std::wstring& db_path) : DatabaseManager(db_path) {}
};