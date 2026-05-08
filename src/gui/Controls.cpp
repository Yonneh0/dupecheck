#include <imgui.h>
#include <windows.h>
#include <shlobj.h>  // For SHBrowseForFolderW / SHGetPathFromIDListW
#include "ImGuiView.h"

// Folder picker button handler.
static void browse_for_folder(std::wstring& path) {
    CoInitialize(nullptr);
    BROWSEINFOW bi = {};
    bi.lpszTitle = L"Select folder";

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != nullptr) {
        wchar_t buffer[MAX_PATH];
        DWORD len = GetLongPathNameW(pidl, buffer, ARRAYSIZE(buffer));
        if (len > 0 && len < ARRAYSIZE(buffer)) {
            path = std::wstring(buffer, len);
        }
        CoTaskMemFree(pidl);
    }
    CoUninitialize();
}

// Render controls panel.
void render_controls(std::wstring& scan_path) {
    wchar_t path_buf[512] = L"";
    if (!scan_path.empty()) {
        wcscpy_s(path_buf, scan_path.c_str());
    }

    if (ImGui::InputText("Scan Path", reinterpret_cast<char*>(path_buf), sizeof(path_buf))) {
        MultiByteToWideChar(CP_UTF8, 0, path_buf, -1, path_buf, ARRAYSIZE(path_buf));
        std::string utf8 = PathUtils::wide_to_utf8(std::wstring(path_buf));
        scan_path = PathUtils::utf8_to_wide(utf8);
    }

    ImGui::SameLine();
    if (ImGui::Button("Browse")) {
        browse_for_folder(scan_path);
    }

    // Scan button.
    ImGui::SameLine();
    if (ImGui::Button("Scan", ImVec2(100, 30))) {
        ImGuiView::start_scan(scan_path.c_str());
    }
}