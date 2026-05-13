#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <imgui.h>
#include <windows.h>
#include <shlobj.h>
#include "Controls.h"
#include "ImGuiView.h"
#include "PreviewPanel.h"
#include "SettingsDialog.h"
#include "../database/DatabaseManager.h"
#include "../hashing/HashEngine.h"

static void browse_for_folder(std::wstring& path) {
    CoInitialize(nullptr);
    BROWSEINFOW bi{};
    bi.lpszTitle = L"Select a folder to scan";
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl != nullptr) {
        wchar_t buffer[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, buffer)) {
            DWORD len = GetLongPathNameW(buffer, nullptr, 0);
            if (len > 0) {
                std::wstring long_buf(len, L'\0');
                if (GetLongPathNameW(buffer, &long_buf[0], len)) {
                    path = std::move(long_buf);
                } else {
                    path.assign(buffer);
                }
            } else {
                path.assign(buffer);
            }
        }
        CoTaskMemFree(pidl);
    }
    CoUninitialize();
}

static bool validate_path(const std::wstring& path) {
    if (path.empty()) {
        MessageBoxW(nullptr, L"Please enter or browse for a folder path.", L"Scan Path", MB_ICONWARNING);
        return false;
    }

    DWORD attrs = GetFileAttributesW(PathUtils::to_long_path(path).c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        std::wstring msg = L"The path \"" + path + L"\" does not exist or is invalid.";
        MessageBoxW(nullptr, msg.c_str(), L"Invalid Path", MB_ICONERROR);
        return false;
    }
    if (!(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        std::wstring msg = L"\"" + path + L"\" is a file, not a folder. Please select a directory.";
        MessageBoxW(nullptr, msg.c_str(), L"Not a Folder", MB_ICONWARNING);
        return false;
    }
    return true;
}

void render_controls(std::wstring& scan_path) {
    wchar_t path_buf[512];
    if (!scan_path.empty()) wcscpy_s(path_buf, scan_path.c_str());
    else path_buf[0] = L'\0';

    const bool path_changed = ImGui::InputText("##path", reinterpret_cast<char*>(path_buf),
                                                static_cast<int>(sizeof(path_buf) / sizeof(wchar_t)) - 1);
    if (path_changed && wcslen(path_buf)) {
        scan_path.assign(path_buf);
    }

    ImGui::SameLine();
    if (ImGui::Button("Browse")) browse_for_folder(scan_path);

    ImGui::SameLine(320, 8);
    if (ImGui::Button("Scan", ImVec2(80, 30))) {
        if (validate_path(scan_path)) {
            ImGuiView::perform_scan(scan_path.c_str());
        }
    }

    if (scan_path.empty()) {
        ImGui::TextDisabled("[No path set — enter a folder or click Browse]");
    } else {
        DWORD attrs = GetFileAttributesW(PathUtils::to_long_path(scan_path).c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.6f, 1.0f));
            ImGui::Text("Path does not exist");
        } else if (!(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.3f, 1.0f));
            ImGui::Text("Not a folder");
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 1.0f, 0.6f, 1.0f));
            ImGui::Text("Path OK");
        }
        ImGui::PopStyleColor();
    }

    render_preview_panel(ImGuiView::get_results());
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 120);
    render_settings_dialog();
}