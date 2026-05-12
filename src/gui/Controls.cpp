#include <imgui.h>
#include <windows.h>
#include "ImGuiView.h"

// Browse for a folder using SHBrowseForFolder dialog.
static void browse_for_folder(std::wstring& path) {
    CoInitialize(nullptr);
    BROWSEINFOW bi{};
    bi.lpszTitle = L"Select folder";
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != nullptr) {
        wchar_t buffer[MAX_PATH];
        DWORD len = GetLongPathNameW(pidl, buffer, ARRAYSIZE(buffer));
        if (len > 0 && len < static_cast<DWORD>(ARRAYSIZE(buffer))) path.assign(buffer, len);
        CoTaskMemFree(pidl);
    }
    CoUninitialize();
}

void render_controls(std::wstring& scan_path) {
    wchar_t path_buf[512];
    if (!scan_path.empty()) wcscpy_s(path_buf, scan_path.c_str());
    else path_buf[0] = L'\0';

    if (ImGui::InputText("##path", reinterpret_cast<char*>(path_buf), sizeof(path_buf))) {
        scan_path.assign(path_buf);
    }
    ImGui::SameLine();
    if (ImGui::Button("Browse")) browse_for_folder(scan_path);
    ImGui::SameLine();
    if (ImGui::Button("Scan", ImVec2(100, 30))) {
        // Trigger a scan with current path.
        (void)scan_path;
    }
}