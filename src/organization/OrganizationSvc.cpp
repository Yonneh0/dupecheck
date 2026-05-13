#include "OrganizationSvc.h"
#include <algorithm>

/// Generate a renamed path with numeric suffix (e.g., "file (1).ext").
std::wstring OrganizationSvc::generate_renamed_path(const FileInfo& file, int index) {
    std::wstring result = PathUtils::get_parent_dir(file.path);
    if (!result.empty() && result.back() != L'\\') result += L"\\";
    return result + PathUtils::get_name_without_ext(file.path) + L" (" + std::to_wstring(index + 1) + L")." + PathUtils::get_extension(file.path);
}

std::vector<ActionItem> OrganizationSvc::generate_actions(const DuplicateGroup& group, ActionType action_type) {
    std::vector<ActionItem> items;
    for (size_t i = 0; i < group.files.size(); ++i) {
        ActionItem item{};
        item.file = group.files[i];
        item.action = action_type;
        item.selected = true;

        if (i == 0) {
            item.type = FileType::Original;
        } else {
            item.type = FileType::Duplicate;
            switch (action_type) {
                case ActionType::Rename:
                    item.new_name = PathUtils::wide_to_utf8(generate_renamed_path(group.files[i], static_cast<int>(i - 1)));
                    item.copy_index = static_cast<int>(i - 1);
                    break;
                case ActionType::MoveToDuplicatesFolder: {
                    std::wstring ext = PathUtils::get_extension(group.files[i].path);
                    std::wstring base_name = PathUtils::get_name_without_ext(group.files[i].path);
                    item.new_name = PathUtils::wide_to_utf8(L"duplicates\\" + (ext.empty() ? base_name : base_name + L"." + ext));
                    break;
                }
                default:
                    break;
            }
        }
        items.push_back(std::move(item));
    }
    return items;
}

void OrganizationSvc::apply_actions(const std::vector<ActionItem>& items) {
    for (const auto& item : items) {
        if (!item.selected) continue;

        ActionHistoryEntry entry{};
        entry.file_path = item.file.path;
        entry.action_type = item.action;

        switch (item.action) {
            case ActionType::Rename: {
                int idx = static_cast<int>(item.copy_index);
                std::wstring new_name = generate_renamed_path(item.file, idx);
                if (MoveFileExW(PathUtils::to_long_path(item.file.path).c_str(),
                               PathUtils::to_long_path(new_name).c_str(), MOVEFILE_REPLACE_EXISTING)) {
                    entry.old_value = PathUtils::wide_to_utf8(item.file.path);
                    entry.new_value = PathUtils::wide_to_utf8(new_name);
                    history_.push_back(entry);
                }
                break;
            }
            case ActionType::MoveToDuplicatesFolder: {
                std::wstring parent_dir = MergeAction::get_duplicates_folder(item.file);
                CreateDirectoryW(parent_dir.c_str(), nullptr);
                std::wstring full_name = PathUtils::get_name_without_ext(item.file.path) + L"." + PathUtils::get_extension(item.file.path);
                if (MoveFileExW(PathUtils::to_long_path(item.file.path).c_str(),
                               PathUtils::to_long_path(parent_dir + L"\\" + full_name).c_str(), MOVEFILE_REPLACE_EXISTING)) {
                    entry.old_value = PathUtils::wide_to_utf8(item.file.path);
                    entry.new_value = PathUtils::wide_to_utf8(parent_dir + L"\\" + full_name);
                    history_.push_back(entry);
                }
                break;
            }
            default:
                break;
        }
    }
}

void OrganizationSvc::undo_one_action(const ActionHistoryEntry& entry) {
    switch (entry.action_type) {
        case ActionType::Rename:
        case ActionType::MoveToDuplicatesFolder:
            // Undo: restore from new_value back to old_value
            MoveFileExW(PathUtils::utf8_to_wide(entry.new_value).c_str(),
                        PathUtils::utf8_to_wide(entry.old_value).c_str(), MOVEFILE_REPLACE_EXISTING);
            break;
    }
}

void OrganizationSvc::redo_one_action(const ActionHistoryEntry& entry) {
    switch (entry.action_type) {
        case ActionType::Rename:
        case ActionType::MoveToDuplicatesFolder:
            // Redo: apply from old_value to new_value again
            MoveFileExW(PathUtils::utf8_to_wide(entry.old_value).c_str(),
                        PathUtils::utf8_to_wide(entry.new_value).c_str(), MOVEFILE_REPLACE_EXISTING);
            break;
    }
}

void OrganizationSvc::undo_actions(int count) {
    while (count > 0 && !history_.empty()) {
        ActionHistoryEntry entry = std::move(history_.back());
        history_.pop_back();
        undo_one_action(entry);
        --count;
    }
}

void OrganizationSvc::clear_history() {
    history_.clear();
}

std::vector<ActionItem> OrganizationSvc::generate_actions(const std::vector<DuplicateGroup>& groups, ActionType action_type) {
    std::vector<ActionItem> all_items;
    for (const auto& group : groups) {
        auto items = generate_actions(group, action_type);
        all_items.insert(all_items.end(), std::make_move_iterator(items.begin()), std::make_move_iterator(items.end()));
    }
    return all_items;
}

void OrganizationSvc::apply(std::vector<ActionItem>& items) { apply_actions(items); }