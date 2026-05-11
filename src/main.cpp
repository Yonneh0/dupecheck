#include <windows.h>
#include "database/DatabaseManager.h"
#include "gui/ImGuiView.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int nCmdShow) {
    HashEngine::init_bcrypt();

    wchar_t appdata[MAX_PATH];
    DWORD len = ExpandEnvironmentStringsW(L"%APPDATA%", appdata, ARRAYSIZE(appdata));
    std::wstring db_path;
    if (len > 0 && len < static_cast<DWORD>(ARRAYSIZE(appdata))) {
        db_path = std::wstring(appdata) + L"\\DupeCheck\\dupecheck.db";
    } else {
        const wchar_t* env = _wgetenv(L"APPDATA");
        if (env) db_path = std::wstring(env) + L"\\DupeCheck\\dupecheck.db";
        else db_path = L"C:\\Windows\\DupeCheck\\dupecheck.db";
    }

    auto dir_pos = db_path.find_last_of(L'\\');
    if (dir_pos != std::wstring::npos) {
        CreateDirectoryW(db_path.substr(0, dir_pos).c_str(), nullptr);
    }

    DatabaseManager db(db_path);
    if (!db.init()) {
        MessageBoxW(nullptr, L"Failed to initialize database.", L"Error", MB_ICONERROR);
        return 1;
    }

    return run_gui(hInstance, nCmdShow, PathUtils::utf8_to_wide(L"C:\\"), db);
}
