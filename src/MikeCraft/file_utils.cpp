#include "file_utils.h"

#include <windows.h>
#include <filesystem>

std::string getExecutableDir()
{
    char  buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (len == 0 || len == MAX_PATH)
        return ".";
    std::string fullPath(buffer, len);
    size_t      pos = fullPath.find_last_of("\\/");
    return (pos == std::string::npos) ? "." : fullPath.substr(0, pos);
}

void ensureWorldFolderExists()
{
    std::filesystem::path worldDir = std::filesystem::path(getExecutableDir()) / "world";
    std::filesystem::create_directories(worldDir);
}

std::string getRegionFilePath(const std::string& regionFileName)
{
    std::filesystem::path regionPath =
        std::filesystem::path(getExecutableDir()) / "world" / regionFileName;
    return regionPath.string();
}