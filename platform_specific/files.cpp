#include "files.h"

#include <cstring>

#include "../debug/logs.h"

using namespace oops_bcode_compiler::platform;
using namespace oops_bcode_compiler::debug;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include "windows.h"

//Copied from StackOverflow https://stackoverflow.com/questions/1387064/how-to-get-the-error-message-from-the-error-code-returned-by-getlasterror/21174331
//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
std::string GetLastErrorAsString()
{
    //Get the error message, if any.
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0)
        return std::string(); //No error message has been recorded

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    std::string message(messageBuffer, size);

    //Free the buffer.
    LocalFree(messageBuffer);

    return message;
}

namespace
{
    std::string normalize_file_name(std::string name, std::string build_path)
    {
        std::string lpcstr;
        lpcstr.reserve(name.length() + build_path.length() + 1);
        lpcstr += build_path;
        if (lpcstr.back() != '/' && lpcstr.back() != '\\')
        {
            lpcstr.back() += '/';
        }
        log.builder(logger::level::debug) << "Class name = " << name << logger::logbuilder::end;
        for (auto c : name)
        {
            lpcstr += c == '.' ? '/' : c;
        }
        log.builder(logger::level::debug) << "Normalized class file root = " << lpcstr << logger::logbuilder::end;
        return lpcstr;
    }
} // namespace

std::optional<file_mapping> oops_bcode_compiler::platform::open_class_file_mapping(std::string name)
{
    std::string lpcstr = ::normalize_file_name(name, platform::get_working_path()) + ".boops";
    log.builder(logger::level::debug) << "Looking for class file " << lpcstr << logger::logbuilder::end;
    void *file_handle = CreateFile(lpcstr.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
    if (file_handle == INVALID_HANDLE_VALUE)
    {
        log.builder(logger::level::error) << "Failed to create file handle because " << GetLastErrorAsString() << logger::logbuilder::end;
        return {};
    }
    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file_handle, &file_size))
    {
        log.builder(logger::level::error) << "Failed to get file size because " << GetLastErrorAsString() << logger::logbuilder::end;
        CloseHandle(file_handle);
        return {};
    }
    void *file_map_handle = CreateFileMapping(file_handle, NULL, PAGE_READONLY, 0, 0, lpcstr.c_str());
    if (file_map_handle == NULL)
    {
        log.builder(logger::level::error) << "Failed to create file mapping handle because " << GetLastErrorAsString() << logger::logbuilder::end;
        CloseHandle(file_handle);
        return {};
    }
    void *mmap_handle = MapViewOfFile(file_map_handle, FILE_MAP_READ, 0, 0, 0);
    if (mmap_handle == NULL)
    {
        log.builder(logger::level::error) << "Failed to map view of file because " << GetLastErrorAsString() << logger::logbuilder::end;
        CloseHandle(file_map_handle);
        CloseHandle(file_handle);
        return {};
    }
    return {{static_cast<char *>(mmap_handle), file_map_handle, file_handle, static_cast<std::size_t>(file_size.QuadPart)}};
}

std::optional<file_mapping> oops_bcode_compiler::platform::create_class_file(std::string name, std::size_t file_size, std::string build_path)
{
    std::string lpcstr = ::normalize_file_name(name, build_path) + ".coops";
    log.builder(logger::level::debug) << "Opening output class file " << lpcstr << logger::logbuilder::end;
    if (void *file_handle = CreateFile(lpcstr.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL); file_handle != NULL)
    {
        if (void *file_mapping_handle = CreateFileMapping(file_handle, NULL, PAGE_READWRITE, file_size & (~static_cast<std::size_t>(0) >> (CHAR_BIT * sizeof(DWORD))), file_size >> (CHAR_BIT * sizeof(DWORD)), lpcstr.c_str()); file_mapping_handle != NULL)
        {
            if (void *mmap_handle = MapViewOfFile(file_mapping_handle, FILE_MAP_WRITE, 0, 0, 0); mmap_handle != NULL)
            {
                return file_mapping{static_cast<char *>(mmap_handle), file_mapping_handle, file_handle, file_size};
            }
            else
            {
                log.builder(logger::level::error) << "Failed to map view of file because " << GetLastErrorAsString() << logger::logbuilder::end;
            }
            CloseHandle(file_mapping_handle);
        }
        else
        {
            log.builder(logger::level::error) << "Failed to create file mapping handle because " << GetLastErrorAsString() << logger::logbuilder::end;
        }
        CloseHandle(file_handle);
    }
    else
    {
        log.builder(logger::level::error) << "Failed to open file mapping because " << GetLastErrorAsString() << logger::logbuilder::end;
    }
    return {};
}

void oops_bcode_compiler::platform::close_file_mapping(file_mapping fm)
{
    UnmapViewOfFile(fm.mmapped_file);
    CloseHandle(fm._file_map_handle);
    CloseHandle(fm._file_handle);
}

const char *oops_bcode_compiler::platform::get_working_path()
{
    return "./";
}

const char *oops_bcode_compiler::platform::get_executable_path()
{
    static char *executable_path = nullptr;
    if (executable_path)
        return executable_path;
    std::size_t executable_size = 8;
    std::size_t string_size = 0;
    do
    {
        char *new_path = static_cast<char *>(std::realloc(executable_path, executable_size <<= 1));
        if (new_path == nullptr)
        {
            std::free(executable_path);
            return executable_path = nullptr;
        }
        string_size = GetModuleFileName(NULL, new_path, executable_size);
    } while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);
    if (string_size == 0)
    {
        std::free(executable_path);
        return executable_path = nullptr;
    }
    char *new_path = static_cast<char *>(std::realloc(executable_path, string_size + 1));
    if (new_path)
    {
        executable_path = new_path;
    }
    log.builder(logger::level::debug) << "Executable path is " << executable_path << logger::logbuilder::end;
    return executable_path;
}

#endif