#include "DbPath.h"
#include <windows.h>

std::wstring get_default_db_path() {
    wchar_t appdata[MAX_PATH]{};
    DWORD len = ExpandEnvironmentStringsW(L"%APPDATA%", appdata, ARRAYSIZE(appdata));
    std::wstring base_dir = (len > 0 && len < static_cast<DWORD>(ARRAYSIZE(appdata))) ? std::wstring(appdata) : L"C:\\Windows";

    std::wstring db_path = base_dir + L"\\DupeCheck\\dupecheck.db";
    auto dir_pos = db_path.find_last_of(L'\\');
    if (dir_pos != std::wstring::npos) {
        CreateDirectoryW(db_path.substr(0, dir_pos).c_str(), nullptr);
    }
    return db_path;
}