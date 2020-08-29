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
        debug::log.builder(debug::logger::level::error) << "File '" << class_file << "' could not be found!" << debug::logger::logbuilder::end;
        return 1;
    }
    debug::log.builder(debug::logger::level::info) << "Successfully found file " << class_file << debug::logger::logbuilder::end;
    if (std::holds_alternative<std::vector<std::string>>(*cls))
    {
        auto errors = std::get<std::vector<std::string>>(*cls);
        debug::log.builder(debug::logger::level::error) << "Tried to parse '" << class_file << "', but got error" << (errors.size() > 1 ? "s" : "") << ":" << debug::logger::logbuilder::end;
        for (auto &error : errors)
        {
            debug::log.builder(debug::logger::level::error) << error << debug::logger::logbuilder::end;
        }
        return errors.size();
    }
    debug::log.builder(debug::logger::level::info) << "Successfully parsed file " << class_file << debug::logger::logbuilder::end;
    if (auto errors = oops_bcode_compiler::transformer::write(std::get<oops_bcode_compiler::parsing::cls>(*cls), build_path); !errors.empty())
    {
        debug::log.builder(debug::logger::level::error) << "Tried to compile and write '" << class_file << "', but got error" << (errors.size() > 1 ? "s" : "") << ":" << debug::logger::logbuilder::end;
        for (auto &error : errors)
        {
            debug::log.builder(debug::logger::level::error) << error << debug::logger::logbuilder::end;
        }
        return errors.size();
    }
    debug::log.builder(debug::logger::level::info) << "Successfully compiled and wrote file " << class_file << debug::logger::logbuilder::end;
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
        debug::log.set_level(debug::logger::level::warning);
    }
    else
    {
        static const std::vector<std::string> levels = {"debug", "info", "warning", "error"};
        if (auto lvl = std::find(levels.begin(), levels.end(), argv[level->second + 1]); lvl != levels.end())
        {
            debug::log.set_level(static_cast<debug::logger::level>(lvl - levels.begin()));
        }
        else
        {
            debug::log.set_level(debug::logger::level::warning);
        }
    }
    auto to_compile = args.find("--file");
    if (to_compile == args.end())
    {
        to_compile = args.find("-f");
    }
    if (to_compile == args.end() or to_compile->second == argc - 1)
    {
        debug::log.builder(debug::logger::level::error) << "No file argument provided!" << debug::logger::logbuilder::end;
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
