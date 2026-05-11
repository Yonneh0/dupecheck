#include <windows.h>
#include <string>
#include "database/DatabaseManager.h"
#include "engine/DuplicateEngine.h"
#include "hashing/HashEngine.h"
#include "gui/ImGuiView.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int nCmdShow) {
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
        wchar_t* dir_copy = new wchar_t[dir_pos + 1];
        memcpy(dir_copy, db_path.c_str(), (dir_pos + 1) * sizeof(wchar_t));
        dir_copy[dir_pos] = L'\0';
        CreateDirectoryW(dir_copy, nullptr);
        delete[] dir_copy;
    }

    DatabaseManager db(db_path);
    if (!db.init()) {
        MessageBoxW(nullptr, L"Failed to initialize database.", L"Error", MB_ICONERROR);
        return 1;
    }

    int result = run_gui(hInstance, nCmdShow, PathUtils::utf8_to_wide(L"C:\\"), db);
    return result;
}

int main(int argc, char** argv) {
    ServiceArgs args = parse_args(argc, argv);
    HashEngine::init_bcrypt();
    std::wstring service_scan_path = args.scan_path.empty() ? L"C:\\" : PathUtils::utf8_to_wide(args.scan_path);
    ServiceHost::run_service(service_scan_path, 300);
    return 0;
}