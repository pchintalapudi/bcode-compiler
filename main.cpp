#include <iostream>
#include <queue>
#include <unordered_set>
#include <unordered_map>

#include "parser/parser.h"
#include "interpreter/translator.h"

int compile_standalone(std::string class_file)
{
    auto cls = oops_bcode_compiler::parsing::parse(class_file);
    if (!cls)
    {
        std::cerr << "File '" << class_file << "' could not be found!" << std::endl;
        return 1;
    }
    if (std::holds_alternative<std::string>(*cls))
    {
        std::cerr << "Tried to parse '" << class_file << "', but got error:\n"
                  << std::get<std::string>(*cls) << std::endl;
        return 1;
    }
    if (auto failure = oops_bcode_compiler::transformer::write(std::get<oops_bcode_compiler::parsing::cls>(*cls)))
    {
        std::cerr << "Failed to fully write '" << class_file << "' due to error:\n"
                  << *failure << std::endl;
        return 1;
    }
    std::cout << "Successfully compiled '" << class_file << "'!" << std::endl;
    return 0;
}

int compile_with_imports(std::string seed)
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
        if (std::holds_alternative<std::string>(*cls))
        {
            std::cerr << "Tried to parse '" << class_file << "', but got error:\n"
                      << std::get<std::string>(*cls) << std::endl;
            continue;
        }
        if (auto failure = oops_bcode_compiler::transformer::write(std::get<oops_bcode_compiler::parsing::cls>(*cls)))
        {
            std::cerr << "Failed to fully write '" << class_file << "' due to error:\n"
                      << *failure << std::endl;
            continue;
        }
        successes.push_back(class_file);
    } while (!to_compile.empty());
    return file_count - successes.size();
}

int main(int argc, char **argv)
{
    std::unordered_map<std::string, std::size_t> args;
    while (argc-- > 0)
    {
        args[argv[argc]] = argc;
    }
    auto to_compile = args.find("--file");
    if (to_compile == args.end())
    {
        to_compile = args.find("-f");
    }
    if (to_compile == args.end())
    {
        std::cerr << "No file argument provided!" << std::endl;
        return 1;
    }
    if (args.find("--compile-imports") != args.end())
    {
        return compile_with_imports(argv[to_compile->second + 1]);
    }
    else
    {
        return compile_standalone(argv[to_compile->second + 1]);
    }
}
