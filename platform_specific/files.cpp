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
                                 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&messageBuffer), 0, NULL);

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
        logger.builder(logging::level::debug) << "Class name = " << name << logging::logbuilder::end;
        for (auto c : name)
        {
            lpcstr += c == '.' ? '/' : c;
        }
        logger.builder(logging::level::debug) << "Normalized class file root = " << lpcstr << logging::logbuilder::end;
        return lpcstr;
    }

    bool prep_directories(const std::string &build_path, const std::string &path)
    {
        std::string lpcstr;
        lpcstr.reserve(path.size());
        lpcstr += build_path;
        for (std::size_t i = build_path.size(); i < path.size(); i++)
        {
            lpcstr += path[i];
            if (path[i] == '/' || path[i] == '\\')
            {
                logger.builder(logging::level::debug) << "Ensuring directory " << lpcstr << " exists" << logging::logbuilder::end;
                if (not CreateDirectory(lpcstr.c_str(), NULL))
                {
                    logger.builder(logging::level::error) << "Failed to create directory " << lpcstr << " because " << GetLastErrorAsString() << logging::logbuilder::end;
                    return false;
                }
            }
        }
        return true;
    }
} // namespace

std::optional<file_mapping> oops_bcode_compiler::platform::open_class_file_mapping(std::string name)
{
    std::string lpcstr = ::normalize_file_name(name, platform::get_working_path()) + ".boops";
    logger.builder(logging::level::debug) << "Looking for class file " << lpcstr << logging::logbuilder::end;
    void *file_handle = CreateFile(lpcstr.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
    if (file_handle == INVALID_HANDLE_VALUE)
    {
        logger.builder(logging::level::error) << "Failed to create file handle because " << GetLastErrorAsString() << logging::logbuilder::end;
        return {};
    }
    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file_handle, &file_size))
    {
        logger.builder(logging::level::error) << "Failed to get file size because " << GetLastErrorAsString() << logging::logbuilder::end;
        CloseHandle(file_handle);
        return {};
    }
    void *file_map_handle = CreateFileMapping(file_handle, NULL, PAGE_READONLY, 0, 0, lpcstr.c_str());
    if (file_map_handle == NULL)
    {
        logger.builder(logging::level::error) << "Failed to create file mapping handle because " << GetLastErrorAsString() << logging::logbuilder::end;
        CloseHandle(file_handle);
        return {};
    }
    void *mmap_handle = MapViewOfFile(file_map_handle, FILE_MAP_READ, 0, 0, 0);
    if (mmap_handle == NULL)
    {
        logger.builder(logging::level::error) << "Failed to map view of file because " << GetLastErrorAsString() << logging::logbuilder::end;
        CloseHandle(file_map_handle);
        CloseHandle(file_handle);
        return {};
    }
    return {{static_cast<char *>(mmap_handle), file_map_handle, file_handle, static_cast<std::size_t>(file_size.QuadPart)}};
}

std::optional<file_mapping> oops_bcode_compiler::platform::create_class_file(std::string name, std::size_t file_size, std::string build_path)
{
    std::string lpcstr = ::normalize_file_name(name, build_path) + ".coops";
    logger.builder(logging::level::debug) << "Opening output class file " << lpcstr << " with size " << file_size << logging::logbuilder::end;
    if (!::prep_directories(build_path, lpcstr))
    {
        return {};
    }
    if (void *file_handle = CreateFile(lpcstr.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL); file_handle != NULL)
    {
        std::uint32_t low = file_size & (~static_cast<std::size_t>(0) >> (CHAR_BIT * sizeof(DWORD))), high = file_size >> (CHAR_BIT * sizeof(DWORD));
        if (void *file_mapping_handle = CreateFileMapping(file_handle, NULL, PAGE_READWRITE, high, low, lpcstr.c_str()); file_mapping_handle != NULL)
        {
            if (void *mmap_handle = MapViewOfFile(file_mapping_handle, FILE_MAP_WRITE, 0, 0, 0); mmap_handle != NULL)
            {
                return file_mapping{static_cast<char *>(mmap_handle), file_mapping_handle, file_handle, file_size};
            }
            else
            {
                logger.builder(logging::level::error) << "Failed to map view of file because " << GetLastErrorAsString() << logging::logbuilder::end;
            }
            CloseHandle(file_mapping_handle);
        }
        else
        {
            logger.builder(logging::level::error) << "Failed to create file mapping handle because " << GetLastErrorAsString() << logging::logbuilder::end;
        }
        CloseHandle(file_handle);
    }
    else
    {
        logger.builder(logging::level::error) << "Failed to open file mapping because " << GetLastErrorAsString() << logging::logbuilder::end;
    }
    return {};
}

void oops_bcode_compiler::platform::close_file_mapping(file_mapping fm, bool flush)
{
    if (flush)
    {
        if (not FlushViewOfFile(fm.mmapped_file, fm.file_size))
        {
            logger.builder(logging::level::error) << "Failed to flush file view to disk because " << GetLastErrorAsString() << logging::logbuilder::end;
        }
    }
    if (not UnmapViewOfFile(fm.mmapped_file))
    {
        logger.builder(logging::level::error) << "Failed to unmap file view because " << GetLastErrorAsString() << logging::logbuilder::end;
    }
    if (not CloseHandle(fm._file_map_handle))
    {
        logger.builder(logging::level::error) << "Failed to close file mapping handle because " << GetLastErrorAsString() << logging::logbuilder::end;
    }
    if (not CloseHandle(fm._file_handle))
    {
        logger.builder(logging::level::error) << "Failed to close file handle because " << GetLastErrorAsString() << logging::logbuilder::end;
    }
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
    logger.builder(logging::level::debug) << "Executable path is " << executable_path << logging::logbuilder::end;
    return executable_path;
}

#endif