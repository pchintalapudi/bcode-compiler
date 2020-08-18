#ifndef PLATFORM_SPECIFIC_FILES
#define PLATFORM_SPECIFIC_FILES
#include <optional>
#include <string>

namespace oops_bcode_compiler
{
    namespace platform
    {
        struct file_mapping {
            char* mmapped_file;
            void* _file_map_handle;
            void* _file_handle;
            std::size_t file_size;
        };

        std::optional<file_mapping> open_class_file_mapping(std::string name);

        std::optional<file_mapping> create_class_file(std::string name, std::uint64_t size, std::string build_path);

        void close_file_mapping(file_mapping fm);

        const char* get_executable_path();
        const char* get_working_path();
    } // namespace platform
} // namespace oops
#endif /* PLATFORM_SPECIFIC_FILES */
