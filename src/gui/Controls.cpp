#include <imgui.h>
#include <windows.h>
#include <shlobj.h>  // For SHBrowseForFolderW / SHGetPathFromIDListW
#include "ImGuiView.h"

// Folder picker button handler.
static void browse_for_folder(std::wstring& path) {
    BROWSEINFOW bi = {};
    bi.lpszTitle = L"Select folder";

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != nullptr) {
        wchar_t buffer[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, buffer)) {
            path = std::wstring(buffer);
        }
        CoTaskMemFree(pidl);
    }
}

// Render controls panel.
void render_controls(std::wstring& scan_path) {
    wchar_t path_buf[512];
    wsprintfW(path_buf, L"%s", scan_path.c_str());

    if (ImGui::InputText("Scan Path", reinterpret_cast<char*>(path_buf), sizeof(path_buf))) {
        std::string utf8 = PathUtils::wide_to_utf8(scan_path);
        MultiByteToWideChar(CP_UTF8, 0, path_buf, -1, path_buf, ARRAYSIZE(path_buf));
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