#include "parser.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <iostream>

using namespace oops_bcode_compiler::parsing;

namespace
{
    struct token
    {
        std::string token;
        std::size_t line_number;
        std::size_t column_number;
    };

    std::vector<token> lex(char *current, char *end)
    {
        std::vector<token> tokens;
        std::size_t line_number, column_number;
        token next = {"", line_number, column_number};
        bool skip = false;
        while (current < end)
        {
            column_number++;
            char c = *current++;
            if (!skip)
            {
                if (std::isspace(c))
                {
                    if (!next.token.empty())
                    {
                        tokens.push_back(std::move(next));
                        next.token.clear();
                    }
                    if (c == '\n')
                    {
                        line_number++;
                    }
                }
                else if (c == ';')
                {
                    skip = true;
                }
                else
                {
                    if (next.token.empty())
                    {
                        next.line_number = line_number;
                        next.column_number = column_number;
                    }
                    next.token += c;
                }
            }
            else
            {
                skip = (c != '\n');
                line_number += !skip;
            }
        }
        if (!next.token.empty())
        {
            tokens.push_back(std::move(next));
        }
        return tokens;
    }

    std::string parse(std::vector<token> &line, bool &in_proc, oops_bcode_compiler::parsing::cls &cls)
    {
#define parse_error(error, line_number, column_number)                                                                \
    std::stringstream error_builder;                                                                                  \
    error_builder << "Parsing error: \"" << error << "\" at line " << line_number << " and column " << column_number; \
    return error_builder.str()
        if (!line.empty())
        {
            std::transform(line[0].token.begin(), line[0].token.end(), line[0].token.begin(), [](unsigned char c) { return std::toupper(c); });
            auto keyword = oops_bcode_compiler::keywords::string_to_keywords.find(line[0].token);
            if (keyword == oops_bcode_compiler::keywords::string_to_keywords.end())
            {
                parse_error(line[0].token << " is not a valid keyword", line[0].line_number, line[0].column_number);
            }
            switch (keyword->second)
            {
#define check_in_proc(key)                                                                                                        \
    case oops_bcode_compiler::keywords::keyword::key:                                                                             \
        if (!in_proc)                                                                                                             \
        {                                                                                                                         \
            parse_error("Expected " << line[0].token << " to be inside a procedure", line[0].line_number, line[0].column_number); \
        }
#define check_out_proc(key)                                                                                                           \
    case oops_bcode_compiler::keywords::keyword::key:                                                                                 \
        if (in_proc)                                                                                                                  \
        {                                                                                                                             \
            parse_error("Expected " << line[0].token << " to not be inside a procedure", line[0].line_number, line[0].column_number); \
        }
#define require_min_args(count)                                                                                                                                                                                \
    if (line.size() < count)                                                                                                                                                                                   \
    {                                                                                                                                                                                                          \
        parse_error("Too few arguments for keyword " << line[0].token << " (" << line.size() << ", expected " << count - 1 << ")", line[0].line_number, line.back().column_number + line.back().token.size()); \
    }
#define require_args(count)                                                                                                                                                                                     \
    require_min_args(count);                                                                                                                                                                                    \
    if (line.size() > count)                                                                                                                                                                                    \
    {                                                                                                                                                                                                           \
        parse_error("Too many arguments for keyword " << line[0].token << " (" << line.size() << ", expected " << count - 1 << ")", line[0].line_number, line.back().column_number + line.back().token.size()); \
    }
                check_out_proc(CLZ)
                {
                    require_args(2);
                    if (cls.imports.size() > 6)
                    {
                        parse_error("Class name must be the first import!", line[0].line_number, line[0].column_number);
                    }
                    cls.imports.push_back(std::move(line[1].token));
                    break;
                }
                check_out_proc(EXT)
                {
                    require_args(2);
                    if (cls.imports.size() > 7)
                    {
                        parse_error("Superclass must be the second import!", line[0].line_number, line[0].column_number);
                    }
                    cls.implement_count++;
                    cls.imports.push_back(std::move(line[1].token));
                    break;
                }
                check_out_proc(IMPL)
                {
                    require_args(2);
                    if (cls.imports.size() > 7 + cls.implement_count)
                    {
                        parse_error("All superinterfaces must come before any other imports!", line[0].line_number, line[0].column_number);
                    }
                    cls.implement_count++;
                    cls.imports.push_back(std::move(line[1].token));
                }
                check_out_proc(IMP)
                {
                    require_args(3);
                    auto type = oops_bcode_compiler::keywords::string_to_keywords.find(line[1].token);
                    if (type == oops_bcode_compiler::keywords::string_to_keywords.end())
                    {
                        parse_error("Second import argument must be CLZ, PROC, IVAR, or SVAR!", line[1].line_number, line[1].column_number);
                    }
                    switch (type->second)
                    {
                    case oops_bcode_compiler::keywords::keyword::CLZ:
                    {
                        cls.imports.push_back(std::move(line[2].token));
                        break;
                    }
                    case oops_bcode_compiler::keywords::keyword::PROC:
                    {
                        std::size_t split_idx = line[2].token.find_last_of('.');
                        cls.methods.push_back({line[2].token.substr(0, split_idx), line[2].token.substr(split_idx + 1)});
                        break;
                    }
                    case oops_bcode_compiler::keywords::keyword::IVAR:
                    {
                        std::size_t split_idx = line[2].token.find_last_of('.');
                        cls.methods.push_back({line[2].token.substr(0, split_idx), line[2].token.substr(split_idx + 1)});
                        break;
                    }
                    case oops_bcode_compiler::keywords::keyword::SVAR:
                    {
                        std::size_t split_idx = line[2].token.find_last_of('.');
                        cls.methods.push_back({line[2].token.substr(0, split_idx), line[2].token.substr(split_idx + 1)});
                        break;
                    }
                    default:
                        parse_error("Second import argument must be CLZ, PROC, IVAR, or SVAR!", line[1].line_number, line[1].column_number);
                    }
                    break;
                }
                check_out_proc(IVAR)
                {
                    require_args(3);
                    cls.instance_variables.push_back({cls.imports[6], line[2].token});
                    cls.self_instances.push_back({std::move(line[1].token), std::move(line[2].token)});
                    break;
                }
                check_out_proc(SVAR)
                {
                    require_args(3);
                    cls.static_variables.push_back({cls.imports[6], line[2].token});
                    cls.self_statics.push_back({std::move(line[1].token), std::move(line[2].token)});
                    break;
                }
                check_out_proc(PROC)
                {
                    require_min_args(3);
                    std::size_t begin;
                    if (line[1].token == "static")
                    {
                        require_min_args(4);
                        cls.self_methods.push_back({line[3].token, line[2].token, {}, {}, true});
                        begin = 4;
                    }
                    else
                    {
                        cls.self_methods.push_back({line[2].token, line[1].token, {}, {}, false});
                        begin = 3;
                    }
                    in_proc = true;
                    if (line.size() > begin)
                    {
                        if ((line.size() - begin) % 2)
                        {
                            parse_error("Arguments must come in pairs of type and name!", line.back().line_number, line.back().column_number);
                        }
                        cls.self_methods.back().parameters.reserve((line.size() - begin) / 2);
                        do
                        {
                            cls.self_methods.back().parameters.push_back({std::move(line[begin].token), std::move(line[begin + 1].token)});
                            if (auto &argname = cls.self_methods.back().parameters.back().name; argname.back() == ',')
                            {
                                argname.pop_back();
                            }
                            begin += 2;
                        } while (begin < line.size());
                    }
                    break;
                }
                typedef oops_bcode_compiler::keywords::keyword kw;
            case kw::ADD:
            case kw::SUB:
            case kw::MUL:
            case kw::DIV:
            case kw::DIVU:
            case kw::ADDI:
            case kw::SUBI:
            case kw::MULI:
            case kw::DIVI:
            case kw::DIVUI:
            case kw::AND:
            case kw::OR:
            case kw::XOR:
            case kw::SLL:
            case kw::SRL:
            case kw::SRA:
            case kw::ANDI:
            case kw::ORI:
            case kw::XORI:
            case kw::SLLI:
            case kw::SRLI:
            case kw::SRAI:
            case kw::BGE:
            case kw::BLT:
            case kw::BLE:
            case kw::BGT:
            case kw::BEQ:
            case kw::BNEQ:
            case kw::BGEI:
            case kw::BLTI:
            case kw::BLEI:
            case kw::BGTI:
            case kw::BEQI:
            case kw::BNEQI:
            case kw::CVLLD:
            case kw::CVLSR:
            case kw::SVLLD:
            case kw::SVLSR:
            case kw::VLLD:
            case kw::VLSR:
            case kw::CALD:
            case kw::CASR:
            case kw::SALD:
            case kw::SASR:
            case kw::ALD:
            case kw::ASR:
            case kw::NEG:
            case kw::IOF:
            case kw::VINV:
                check_in_proc(IINV)
                {
                    require_args(4);
                    cls.self_methods.back().instructions.push_back({std::move(line[2].token), std::move(line[1].token), std::move(line[0].token), keyword->second});
                    break;
                }
            case kw::SINV:
            case kw::CSTLD:
            case kw::CSTSR:
            case kw::SSTLD:
            case kw::SSTSR:
            case kw::STLD:
            case kw::STSR:
            case kw::LI:
            case kw::CST:
            case kw::ALEN:
            case kw::VNEW:
            case kw::ANEW:
                check_in_proc(DEF)
                {
                    require_args(3);
                    cls.self_methods.back().instructions.push_back({std::move(line[2].token), std::move(line[1].token), "", keyword->second});
                    break;
                }
            case kw::PASS:
            case kw::RET:
            case kw::LBL:
                check_in_proc(BU)
                {
                    require_args(2);
                    cls.self_methods.back().instructions.push_back({std::move(line[2].token), "", "", keyword->second});
                    break;
                }
            case kw::BCMP:
            case kw::BADR:
            case kw::NOP:
            case kw::EXC:
                parse_error(line[0].token << " is a keyword not implemented in the parser", line[0].line_number, line[0].column_number);
            }
        }
        return "";
    }
} // namespace

std::optional<std::variant<cls, std::vector<std::string>>> oops_bcode_compiler::parsing::parse(std::string filename)
{
    auto mapping = platform::open_class_file_mapping(filename);
    if (!mapping)
    {
        return {};
    }
    cls ret;
    for (auto imp : {"char", "short", "int", "long", "float", "double"})
    {
        ret.imports.emplace_back(imp);
    }
    ret.implement_count = ret.static_method_count = 0;
    char *current = mapping->mmapped_file, *end = current + mapping->file_size;
    bool in_proc = false;
    std::vector<std::string> errors;
    std::vector<token> line;
    for (auto &&token : ::lex(current, end))
    {
        if (!line.empty() and line.back().line_number != token.line_number)
        {
            if (auto error = ::parse(line, in_proc, ret); !error.empty())
            {
                errors.push_back(std::move(error));
            }
            line.clear();
        }
        line.push_back(std::move(token));
    }
    if (!line.empty())
    {
        ::parse(line, in_proc, ret);
    }
    platform::close_file_mapping(*mapping);
    if (errors.empty())
    {
        return ret;
    }
    else
    {
        return errors;
    }
}