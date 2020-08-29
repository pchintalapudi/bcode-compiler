#include <iostream>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include "parser/parser.h"
#include "interpreter/translator.h"
#include "platform_specific/files.h"

int compile_standalone(std::string class_file, std::string build_path)
{
    auto cls = oops_bcode_compiler::parsing::parse(class_file);
    if (!cls)
    {
        std::cerr << "File '" << class_file << "' could not be found!" << std::endl;
        return 1;
    }
    if (std::holds_alternative<std::vector<std::string>>(*cls))
    {
        auto errors = std::get<std::vector<std::string>>(*cls);
        std::cerr << "Tried to parse '" << class_file << "', but got error" << (errors.size() > 1 ? "s" : "") << ":\n";
        for (auto &error : errors)
        {
            std::cerr << error << "\n";
        }
        return errors.size();
    }
    if (auto failure = oops_bcode_compiler::transformer::write(std::get<oops_bcode_compiler::parsing::cls>(*cls), build_path); !failure.empty())
    {
        std::cerr << "Failed to fully write '" << class_file << "' due to errors:\n";
        for (auto &fail : failure)
        {
            std::cerr << fail << "\n";
        }
        return 1;
    }
    std::cout << "Successfully compiled '" << class_file << "'!" << std::endl;
    return 0;
}

int compile_with_imports(std::string seed, std::string build_path)
{
    std::queue<std::string> to_compile;
    std::unordered_set<std::string> compiling;
    std::vector<std::string> successes;
    std::size_t file_count = 0;
    to_compile.push(seed);
    do
    {
        file_count++;
        auto class_file = to_compile.front();
        to_compile.pop();
        auto cls = oops_bcode_compiler::parsing::parse(class_file);
        if (!cls)
        {
            std::cerr << "File '" << class_file << "' could not be found!" << std::endl;
            continue;
        }
        if (std::holds_alternative<std::vector<std::string>>(*cls))
        {
            auto errors = std::get<std::vector<std::string>>(*cls);
            std::cerr << "Tried to parse '" << class_file << "', but got error" << (errors.size() > 1 ? "s" : "") << ":\n";
            for (auto &error : errors)
            {
                std::cerr << error << "\n";
            }
            continue;
        }
        for (auto &imp : std::get<oops_bcode_compiler::parsing::cls>(*cls).imports)
        {
            if (compiling.find(imp.name) == compiling.end())
            {
                to_compile.push(imp.name);
                compiling.insert(imp.name);
            }
        }
        if (auto failure = oops_bcode_compiler::transformer::write(std::get<oops_bcode_compiler::parsing::cls>(*cls), build_path); !failure.empty())
        {
            std::cerr << "Failed to fully write '" << class_file << "' due to errors:\n";
            for (auto &fail : failure)
            {
                std::cout << fail << "\n";
            }
            continue;
        }
        std::cout << "Successfully compiled class file " << class_file;
        successes.push_back(class_file);
    } while (!to_compile.empty());
    return file_count - successes.size();
}

int main(int argc, char **argv)
{
    std::unordered_map<std::string, int> args;
    while (argc-- > 0)
    {
        args[argv[argc]] = argc;
    }
    auto to_compile = args.find("--file");
    if (to_compile == args.end())
    {
        to_compile = args.find("-f");
    }
    if (to_compile == args.end() or to_compile->second == argc - 1)
    {
        std::cerr << "No file argument provided!" << std::endl;
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

    if (args.find("--compile-imports") != args.end())
    {
        return compile_with_imports(argv[to_compile->second + 1], build_path);
    }
    else
    {
        return compile_standalone(argv[to_compile->second + 1], build_path);
    }
}
