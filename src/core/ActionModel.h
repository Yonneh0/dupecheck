#pragma once

#include <vector>
#include <unordered_map>
#include "../core/FileInfo.h"
#include "../hashing/HashEngine.h"

// Compute SHA256 of a directory's contents (names + sizes).
inline void compute_tree_hash(const std::wstring& dir_path, Sha256& out_hash) {
    static std::once_flag init_flag;
    std::call_once(init_flag, []() { HashEngine::init_bcrypt(); });

    std::vector<FileInfo> entries;
    PathUtils::enumerate_files(dir_path, entries);

    if (entries.empty()) {
        std::fill(out_hash.begin(), out_hash.end(), 0);
        return;
    }

    std::vector<std::pair<std::wstring, uint64_t>> file_entries(entries.size());
    for (size_t i = 0; i < entries.size(); ++i) {
        size_t last_sep = entries[i].path.find_last_of(L'\\');
        file_entries[i] = {entries[i].path.substr(last_sep + 1), entries[i].size};
    }
    std::sort(file_entries.begin(), file_entries.end());

    std::string sha_input;
    sha_input.reserve(128 * entries.size());
    for (const auto& [name, sz] : file_entries) {
        sha_input += PathUtils::wide_to_utf8(name) + "|" + std::to_string(sz) + "\n";
    }

    BCRYPT_ALG_HANDLE alg = HashEngine::get_alg_handle();
    if (!alg) return;

    BCRYPT_HASH_HANDLE hHash = nullptr;
    NTSTATUS status = BCryptCreateHash(alg, &hHash, nullptr, 0, nullptr, 0, 0);
    if (status == ERROR_SUCCESS) {
        BCryptHashData(hHash, reinterpret_cast<uint8_t*>(const_cast<char*>(sha_input.data())), static_cast<DWORD>(sha_input.size()), 0);
        BCryptFinishHash(hHash, out_hash.data(), static_cast<DWORD>(out_hash.size()), 0);
        BCryptDestroyHash(hHash);
    } else {
        std::fill(out_hash.begin(), out_hash.end(), static_cast<uint8_t>(dir_path.size() & 0xFF));
    }
}

// Group directories with identical tree hashes.
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
            FileInfo fi{p};
            dg.files.push_back(std::move(fi));
        }
        dg.strategy = Strategy::FolderCopy;
        dg.label = "Folder Copy (" + PathUtils::wide_to_utf8(PathUtils::get_name_without_ext(path_list[0])) + ")";
        groups.push_back(std::move(dg));
    }
    return groups;
}