#include "path_utils.h"

#ifdef _WIN32

#include <windows.h>

std::filesystem::path getExecutableDirectory() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);

    return std::filesystem::path(buffer).parent_path();
}

#endif

#ifdef __linux__

#include <unistd.h>

std::filesystem::path getExecutableDirectory() {
    char buffer[1024];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer));

    return std::filesystem::path(std::string(buffer, len)).parent_path();
}

#endif
