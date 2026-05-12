#pragma once
#include <string>
#include <unordered_map>

class ExtensionFamilyMap {
public:
    static std::wstring get_family(const std::wstring& ext) {
        auto it = builtin_families().find(ext);
        return (it != builtin_families().end()) ? it->second : ext;
    }

    static bool is_same_family(const std::wstring& a, const std::wstring& b) {
        return get_family(a) == get_family(b);
    }

private:
    ExtensionFamilyMap() = delete;
    static const std::unordered_map<std::wstring, std::wstring>& builtin_families();
};

inline const std::unordered_map<std::wstring, std::wstring>& ExtensionFamilyMap::builtin_families() {
    static const std::unordered_map<std::wstring, std::wstring> families = {
        {L"jpg", L"image"},   {L"jpeg", L"image"},  {L"jpe", L"image"},
        {L"png", L"image"},   {L"gif", L"image"},   {L"bmp", L"image"},
        {L"tiff", L"image"},  {L"tif", L"image"},   {L"webp", L"image"},

        {L"docx", L"document"},  {L"doc", L"document"}, {L"docm", L"document"},
        {L"xlsx", L"spreadsheet"}, {L"xls", L"spreadsheet"}, {L"csv", L"spreadsheet"},

        {L"zip", L"archive"}, {L"rar", L"archive"},   {L"7z", L"archive"},
        {L"tar", L"archive"},  {L"gzip", L"archive"},

        {L"mp4", L"video"}, {L"avi", L"video"}, {L"mkv", L"video"},
        {L"mov", L"video"},  {L"wmv", L"video"},

        {L"mp3", L"audio"}, {L"wav", L"audio"}, {L"ogg", L"audio"},
        {L"m4a", L"audio"}, {L"aac", L"audio"}, {L"flac", L"audio"},
    };
    return families;
}