#include "OrganizationSvc.h"
#include "../database/DatabaseManager.h"
#include <algorithm>
#include <shlwapi.h>

std::vector<ActionHistoryEntry> OrganizationSvc::history_;

std::wstring OrganizationSvc::generate_renamed_path(const FileInfo& file, int index) {
    auto name = PathUtils::get_name_without_ext(file.path);
    auto ext = PathUtils::get_extension(file.path);
    auto parent_dir = PathUtils::get_parent_dir(file.path);

    std::wstring result;
    if (!parent_dir.empty()) {
        result = parent_dir + L"\\";
    }
    return result + name + L" (" + std::to_wstring(index + 1) + L")." +
           (!ext.empty() ? ext : L"");
}

std::vector<ActionItem> OrganizationSvc::generate_actions(const DuplicateGroup& group, ActionType action_type) {
    std::vector<ActionItem> items;

    for (size_t i = 0; i < group.files.size(); ++i) {
        ActionItem item;
        item.file = group.files[i];

        if (i == 0) {
            item.type = FileType::Original;
        } else {
            item.type = FileType::Duplicate;

            switch (action_type) {
                case ActionType::Rename: {
                    std::wstring new_name_full = OrganizationSvc::generate_renamed_path(group.files[i], static_cast<int>(i - 1));
                    item.new_name = PathUtils::wide_to_utf8(new_name_full);
                    item.copy_index = static_cast<int>(i - 1);
                    break;
                }
                case ActionType::MoveToDuplicatesFolder: {
                    std::wstring ext = PathUtils::get_extension(group.files[i].path);
                    std::wstring base_name = PathUtils::get_name_without_ext(group.files[i].path);
                    item.new_name = "duplicates/" + (ext.empty() ? base_name : base_name + L"." + ext);
                    break;
                }
                case ActionType::Delete:
                    item.new_name = "(delete)";
                    break;
            }
        }

        item.action = action_type;
        item.selected = true;
        items.push_back(std::move(item));
    }

    return items;
}

void OrganizationSvc::apply_actions(const std::vector<ActionItem>& items) {
    for (const auto& item : items) {
        if (!item.selected) continue;

        ActionHistoryEntry entry;
        entry.file_path = item.file.path;

        switch (item.action) {
            case ActionType::Rename: {
                int idx = item.copy_index > 0 ? item.copy_index : 1;
                std::wstring new_name = OrganizationSvc::generate_renamed_path(item.file, idx);
                std::wstring old_name = PathUtils::to_long_path(item.file.path);

                if (MoveFileExW(old_name.c_str(),
                               PathUtils::to_long_path(new_name).c_str(),
                               MOVEFILE_REPLACE_EXISTING)) {
                    entry.old_value = PathUtils::wide_to_utf8(old_name);
                    entry.new_value = PathUtils::wide_to_utf8(new_name);
                }

                break;
            }

            case ActionType::MoveToDuplicatesFolder: {
                std::wstring dup_dir = MoveAction::get_duplicates_folder(
                    PathUtils::get_parent_dir(item.file.path));
                CreateDirectoryW(dup_dir.c_str(), nullptr);

                std::wstring full_name = PathUtils::get_name_without_ext(item.file.path) +
                                         L"." + PathUtils::get_extension(item.file.path);

                if (MoveFileExW(PathUtils::to_long_path(item.file.path).c_str(),
                               PathUtils::to_long_path(dup_dir + L"\\" + full_name).c_str(),
                               MOVEFILE_REPLACE_EXISTING)) {
                    entry.old_value = PathUtils::wide_to_utf8(PathUtils::to_long_path(item.file.path));
                    entry.new_value = PathUtils::wide_to_utf8(dup_dir + L"\\" + full_name);
                }

                break;
            }

            case ActionType::Delete: {
                if (DeleteFileW(PathUtils::to_long_path(item.file.path).c_str())) {
                    entry.old_value = PathUtils::wide_to_utf8(
                        PathUtils::to_long_path(item.file.path));
                    entry.new_value = "(deleted)";

                    entry.backup_path = item.file.path;
                }

                break;
            }

            case ActionType::CreateSymlink: {
                std::wstring link_name = PathUtils::to_long_path(
                    PathUtils::get_parent_dir(item.file.path) + L"\\_" +
                    PathUtils::get_name_without_ext(item.file.path) + L".link");
                if (CreateSymbolicLinkW(link_name.c_str(),
                                       PathUtils::to_long_path(item.file.path).c_str(),
                                       FILE_ATTRIBUTE_NORMAL)) {
                    entry.old_value = L"";
                    entry.new_value = PathUtils::wide_to_utf8(link_name);
                }

                break;
            }

            case ActionType::Archive:
            case ActionType::MoveToDuplicatesFolder:
                break;
        }

        entry.action_type = item.action;
        history_.push_back(entry);
    }
}

void OrganizationSvc::undo_actions() {
    if (history_.empty()) return;

    auto entry = std::move(history_.back());
    history_.pop_back();

    switch (entry.action_type) {
        case ActionType::Rename: {
            MoveFileExW(PathUtils::utf8_to_wide(entry.new_value).c_str(),
                       PathUtils::utf8_to_wide(entry.old_value).c_str(),
                       MOVEFILE_REPLACE_EXISTING);
            break;
        }

        case ActionType::MoveToDuplicatesFolder: {
            MoveFileExW(PathUtils::utf8_to_wide(entry.new_value).c_str(),
                       PathUtils::utf8_to_wide(entry.old_value).c_str(),
                       MOVEFILE_REPLACE_EXISTING);
            break;
        }

        case ActionType::Delete: {
            if (!entry.backup_path.empty()) {
                HANDLE h = CreateFileW(PathUtils::to_long_path(entry.backup_path).c_str(),
                                       GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                                       CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);

                if (h != INVALID_HANDLE_VALUE) {
                    DWORD bytes_written;
                    const wchar_t marker[] = L"[restored by DupeCheck]";
                    WriteFile(h, marker, sizeof(marker), &bytes_written, nullptr);
                    CloseHandle(h);
                }
            }

            break;
        }

        case ActionType::CreateSymlink: {
            DeleteFileW(PathUtils::utf8_to_wide(entry.new_value).c_str());
            break;
        }

        default:
            if (entry.old_value.empty()) {
                DeleteFileW(PathUtils::utf8_to_wide(entry.new_value));
            } else {
                MoveFileExW(
                    PathUtils::utf8_to_wide(entry.old_value).c_str(),
                    PathUtils::utf8_to_wide(entry.new_value).c_str(),
                    MOVEFILE_REPLACE_EXISTING);
            }
            break;
    }
}

void OrganizationSvc::undo_all() {
    while (!history_.empty()) {
        undo_actions();
    }
}

std::vector<ActionItem> OrganizationSvc::generate_actions(const std::vector<DuplicateGroup>& groups,
                                                          ActionType action_type) {
    std::vector<ActionItem> all_items;
    for (const auto& group : groups) {
        auto items = generate_actions(group, action_type);
        all_items.insert(all_items.end(),
                         std::make_move_iterator(items.begin()),
                         std::make_move_iterator(items.end()));
    }
    return all_items;
}

void OrganizationSvc::apply(std::vector<ActionItem>& items) {
    apply_actions(items);
}