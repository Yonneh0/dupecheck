#include <imgui.h>
#include <windows.h>
#include "Controls.h"
#include "ImGuiView.h"
#include "PreviewPanel.h"
#include "SettingsDialog.h"
#include "../database/DatabaseManager.h"
#include "../hashing/HashEngine.h"

// Browse for a folder using SHBrowseForFolder dialog.
static void browse_for_folder(std::wstring& path) {
    CoInitialize(nullptr);
    BROWSEINFOW bi{};
    bi.lpszTitle = L"Select a folder to scan";
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != nullptr) {
        wchar_t buffer[MAX_PATH];
        DWORD len = GetLongPathNameW(pidl, buffer, ARRAYSIZE(buffer));
        if (len > 0 && len < static_cast<DWORD>(ARRAYSIZE(buffer))) path.assign(buffer, len);
        CoTaskMemFree(pidl);
    }
    CoUninitialize();
}

/// Validate that the given path exists and is a directory. Returns true if valid.
static bool validate_path(const std::wstring& path) {
    if (path.empty()) {
        MessageBoxW(nullptr, L"Please enter or browse for a folder path.", "Scan Path", MB_ICONWARNING);
        return false;
    }

    DWORD attrs = GetFileAttributesW(PathUtils::to_long_path(path).c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        std::wstring msg = L"The path \"" + path + L"\" does not exist or is invalid.";
        MessageBoxW(nullptr, msg.c_str(), "Invalid Path", MB_ICONERROR);
        return false;
    }
    if (!(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        std::wstring msg = L"\"" + path + L"\" is a file, not a folder. Please select a directory.";
        MessageBoxW(nullptr, msg.c_str(), "Not a Folder", MB_ICONWARNING);
        return false;
    }
    return true;
}

void render_controls(std::wstring& scan_path) {
    // Path input field — pressing Enter triggers a scan.
    wchar_t path_buf[512];
    if (!scan_path.empty()) wcscpy_s(path_buf, scan_path.c_str());
    else path_buf[0] = L'\0';

    const bool path_changed = ImGui::InputText("##path", reinterpret_cast<char*>(path_buf), static_cast<int>(sizeof(path_buf) / sizeof(wchar_t)));
    if (path_changed && wcslen(path_buf)) {
        scan_path.assign(path_buf);
    }

    // Browse button.
    ImGui::SameLine();
    if (ImGui::Button("Browse")) browse_for_folder(scan_path);

    // Scan button — validates path then triggers a full scan.
    ImGui::SameLine(320, 8);
    if (ImGui::Button("Scan", ImVec2(80, 30))) {
        if (validate_path(scan_path)) {
            ImGuiView::perform_scan(scan_path.c_str());
        }
    }

    // Path status indicator — positioned below controls to persist.
    if (scan_path.empty()) {
        ImGui::TextDisabled("[No path set — enter a folder or click Browse]");
    } else {
        DWORD attrs = GetFileAttributesW(PathUtils::to_long_path(scan_path).c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.6f, 1.0f));
            ImGui::Text("Path does not exist");
            ImGui::PopStyleColor();
        } else if (!(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.3f, 1.0f));
            ImGui::Text("Not a folder");
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 1.0f, 0.6f, 1.0f));
            ImGui::Text("Path OK");
            ImGui::PopStyleColor();
        }
    }

    // Render preview panel below controls.
    render_preview_panel(ImGuiView::get_results());

    // Settings button in the corner (rendered by its own function).
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 120);
    render_settings_dialog();
}