#pragma once
#include <vector>
#include <unordered_map>
#include "../core/FileInfo.h"
#include "../hashing/HashEngine.h"

/// Compute a tree hash for a directory by hashing all file entries.
static void compute_tree_hash(const std::wstring& dir_path, Sha256& out_hash) {
    HashEngine::init_bcrypt();

    // Enumerate files recursively under this directory.
    std::vector<FileInfo> entries;
    PathUtils::enumerate_files(dir_path, entries);

    if (entries.empty()) {
        std::fill(out_hash.begin(), out_hash.end(), 0);
        return;
    }

    // Build a sorted list of relative paths and sizes.
    struct Entry {
        std::wstring rel_path;
        uint64_t size;
    };
    std::vector<Entry> file_entries;
    file_entries.reserve(entries.size());
    for (const auto& entry : entries) {
        const wchar_t* prefix = dir_path.c_str();
        if (_wcsnicmp(entry.path.c_str(), prefix, dir_path.length()) == 0) {
            file_entries.push_back({entry.path.substr(dir_path.length()), entry.size});
        } else {
            std::filesystem::path fp(entry.path);
            file_entries.push_back({fp.stem().wstring(), entry.size});
        }
    }
    std::sort(file_entries.begin(), file_entries.end(),
              [](const Entry& a, const Entry& b) { return a.rel_path < b.rel_path; });

    // Serialize into a single string and hash with SHA256.
    std::string sha_input;
    sha_input.reserve(128 * entries.size());
    for (const auto& [rel_path, sz] : file_entries) {
        sha_input += PathUtils::wide_to_utf8(rel_path) + "|" + std::to_string(sz) + "\n";
    }

    BCRYPT_HASH_HANDLE hHash = nullptr;
    NTSTATUS status = BCryptCreateHash(HashEngine::get_alg_handle(), &hHash, nullptr, 0, nullptr, 0, 0);
    if (status == ERROR_SUCCESS) {
        BCryptHashData(hHash, reinterpret_cast<uint8_t*>(const_cast<char*>(sha_input.data())),
                       static_cast<DWORD>(sha_input.size()), 0);
        BCryptFinishHash(hHash, out_hash.data(), static_cast<DWORD>(out_hash.size()), 0);
        BCryptDestroyHash(hHash);
    } else {
        std::fill(out_hash.begin(), out_hash.end(), static_cast<uint8_t>(dir_path.size()));
    }
}

/// Group directories with identical tree hashes (likely copies of each other).
inline std::vector<DuplicateGroup> folder_copy(const std::vector<std::wstring>& dirs) {
    std::unordered_map<Sha256, std::vector<std::wstring>> hash_to_dirs;
    for (const auto& d : dirs) {
        Sha256 h{};
        compute_tree_hash(d, h);
        hash_to_dirs[h].push_back(d);
    }

    std::vector<DuplicateGroup> groups;
    for (auto& [hash, path_list] : hash_to_dirs) {
        if (path_list.size() < 2) continue;
        DuplicateGroup dg;
        for (const auto& p : path_list) {
            FileInfo fi{p, 0, 0};
            dg.files.push_back(std::move(fi));
        }
        dg.strategy = Strategy::FolderCopy;
        dg.label = "Folder Copy (" + PathUtils::wide_to_utf8(PathUtils::get_name_without_ext(path_list[0]))
                   + " x" + std::to_string(path_list.size()) + ")";
        groups.push_back(std::move(dg));
    }
    return groups;
}