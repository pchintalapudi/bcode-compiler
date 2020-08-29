#include <algorithm>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include "debug/logs.h"
#include "parser/parser.h"
#include "interpreter/translator.h"
#include "platform_specific/files.h"

using namespace oops_bcode_compiler;

int compile_standalone(std::string class_file, std::string build_path)
{
    auto cls = oops_bcode_compiler::parsing::parse(class_file);
    if (!cls)
    {
        debug::logger.builder(debug::logging::level::error) << "File '" << class_file << "' could not be found!" << debug::logging::logbuilder::end;
        return 1;
    }
    debug::logger.builder(debug::logging::level::info) << "Successfully found file " << class_file << debug::logging::logbuilder::end;
    if (std::holds_alternative<std::vector<std::string>>(*cls))
    {
        auto errors = std::get<std::vector<std::string>>(*cls);
        debug::logger.builder(debug::logging::level::error) << "Tried to parse '" << class_file << "', but got error" << (errors.size() > 1 ? "s" : "") << ":" << debug::logging::logbuilder::end;
        for (auto &error : errors)
        {
            debug::logger.builder(debug::logging::level::error) << error << debug::logging::logbuilder::end;
        }
        return errors.size();
    }
    debug::logger.builder(debug::logging::level::info) << "Successfully parsed file " << class_file << debug::logging::logbuilder::end;
    if (auto errors = oops_bcode_compiler::transformer::write(std::get<oops_bcode_compiler::parsing::cls>(*cls), build_path); !errors.empty())
    {
        debug::logger.builder(debug::logging::level::error) << "Tried to compile and write '" << class_file << "', but got error" << (errors.size() > 1 ? "s" : "") << ":" << debug::logging::logbuilder::end;
        for (auto &error : errors)
        {
            debug::logger.builder(debug::logging::level::error) << error << debug::logging::logbuilder::end;
        }
        return errors.size();
    }
    debug::logger.builder(debug::logging::level::info) << "Successfully compiled and wrote file " << class_file << debug::logging::logbuilder::end;
    return 0;
}

int main(int argc, char **argv)
{
    std::unordered_map<std::string, int> args;
    while (argc-- > 0)
    {
        args[argv[argc]] = argc;
    }
    auto level = args.find("--log-level");
    if (level == args.end() || level->second == argc - 1)
    {
        debug::logger.set_level(debug::logging::level::warning);
    }
    else
    {
        static const std::vector<std::string> levels = {"debug", "info", "warning", "error"};
        if (auto lvl = std::find(levels.begin(), levels.end(), argv[level->second + 1]); lvl != levels.end())
        {
            debug::logger.set_level(static_cast<debug::logging::level>(lvl - levels.begin()));
        }
        else
        {
            debug::logger.set_level(debug::logging::level::warning);
        }
    }
    auto to_compile = args.find("--file");
    if (to_compile == args.end())
    {
        to_compile = args.find("-f");
    }
    if (to_compile == args.end() or to_compile->second == argc - 1)
    {
        debug::logger.builder(debug::logging::level::error) << "No file argument provided!" << debug::logging::logbuilder::end;
        return 1;
    }
    std::string build_path;
    auto out_dir = args.find("--build-path");
    if (out_dir == args.end())
    {
        out_dir = args.find("-b");
    }
    if (out_dir == args.end() or out_dir->second == argc - 1)
    {
        build_path = oops_bcode_compiler::platform::get_working_path();
    }
    else
    {
        build_path = argv[out_dir->second + 1];
    }
    return compile_standalone(argv[to_compile->second + 1], build_path);
}
