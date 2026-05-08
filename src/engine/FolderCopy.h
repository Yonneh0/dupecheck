#pragma once
#include <vector>
#include <unordered_map>
#include <thread>
#include "../core/FileInfo.h"
#include "DuplicateEngine.h"

// Strategy 5e: Detect directory tree copies using tree hashing.

// Compute SHA256 tree hash for a directory by hashing sorted filename|size lines.
inline void compute_tree_hash(const std::wstring& dir_path, Sha256& out_hash) {
    // Collect all files in the directory tree using enumerate_files.
    std::vector<PathUtils::FileEntry> entries;
    PathUtils::enumerate_files(dir_path, entries);

    if (entries.empty()) {
        // Empty directory gets a zero hash.
        std::fill(out_hash.begin(), out_hash.end(), 0);
        return;
    }

    // Sort by filename for deterministic ordering.
    std::vector<std::pair<std::wstring, uint64_t>> file_entries;
    file_entries.reserve(entries.size());
    for (auto& entry : entries) {
        const std::wstring* p = &entry.path;
        size_t last_sep = p->find_last_of(L'\\');
        std::wstring name = (last_sep != std::wstring::npos) ? p->substr(last_sep + 1) : *p;
        file_entries.push_back({name, entry.size});
    }

    std::sort(file_entries.begin(), file_entries.end());

    // Build a string to hash: "filename|size\n" per file.
    std::string sha_input;
    sha_input.reserve(128 * entries.size());
    for (const auto& [name, sz] : file_entries) {
        const std::string n = PathUtils::wide_to_utf8(name);
        sha_input += n + "|" + std::to_string(sz) + "\n";
    }

    // Compute SHA256 using BCRYPT.
    BCRYPT_ALG_HANDLE hAlg{nullptr};
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (status != ERROR_SUCCESS) {
        std::fill(out_hash.begin(), out_hash.end(), static_cast<uint8_t>(dir_path.size()));
        return;
    }

    BCRYPT_HASH_HANDLE hHash = nullptr;
    status = BCryptCreateHash(hAlg, &hHash, nullptr, 0, nullptr, 0, 0);
    if (status == ERROR_SUCCESS) {
        BCryptHashData(hHash,
                       reinterpret_cast<uint8_t*>(const_cast<char*>(sha_input.data())),
                       static_cast<DWORD>(sha_input.size()), 0);

        DWORD hashLen = sizeof(out_hash);
        BCryptFinishHash(hHash, out_hash.data(), static_cast<DWORD>(out_hash.size()), 0);
        BCryptDestroyHash(hHash);
    }
    BCryptCloseAlgorithmProvider(hAlg, 0);
}

inline std::vector<DuplicateGroup> folder_copy(const std::vector<std::wstring>& dirs) {
    // Compute tree hashes for all directories.
    // Map from hash -> list of directory paths so we merge duplicates properly.
    std::unordered_map<Sha256, std::vector<std::wstring>> hash_to_dirs;

    for (const auto& d : dirs) {
        Sha256 h{};
        compute_tree_hash(d, h);
        hash_to_dirs[h].push_back(d);
    }

    // Build groups: only emit a group if multiple directories share the same tree hash.
    std::vector<DuplicateGroup> groups;
    for (const auto& [hash, path_list] : hash_to_dirs) {
        if (path_list.size() < 2) continue;

        DuplicateGroup dg;
        for (const auto& p : path_list) {
            FileInfo fi{p, 0, 0};
            dg.files.push_back(std::move(fi));
        }
        dg.strategy = Strategy::FolderCopy;

        std::wstring name_no_ext = PathUtils::get_name_without_ext(path_list[0]);
        dg.label = "Folder Copy (" + PathUtils::wide_to_utf8(name_no_ext) + " x" +
                   std::to_string(path_list.size()) + ")";
        groups.push_back(std::move(dg));
    }

    return groups;
}