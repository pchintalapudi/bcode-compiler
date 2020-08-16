#include "parser.h"

#include <algorithm>
#include <cctype>
#include <sstream>

using namespace oops_bcode_compiler::parsing;

parser::parser(std::string filename)
{
    if (auto mapping = platform::open_class_file_mapping(filename))
    {
        this->mapping = *mapping;
    }
    else
    {
        this->mapping.mmapped_file = nullptr;
    }
}

parser::~parser()
{
    if (this->mapping.mmapped_file)
    {
        platform::close_file_mapping(this->mapping);
    }
}

namespace
{

#define parsing_error(message)                                         \
    error_builder << message << " (at Line: "                          \
                  << line_number << ", Col: " << column_number << ")"; \
    return error_builder.str()

#define unexpected_eof parsing_error("Unexpected EOF while parsing import")

#define unexpected_eol parsing_error("Unexpected end of line while parsing import")

#define skip_whitespace  \
    do                   \
    {                    \
        column_number++; \
        current++;       \
    } while (current < end and *current != '\n' and std::isspace(*current))

#define guard_end                            \
    if (current == end)                      \
    {                                        \
        unexpected_eof;                      \
    }                                        \
    if (*current == ';' or *current == '\n') \
    {                                        \
        unexpected_eol;                      \
    }

#define maybe_empty(action, nonempty)                 \
    if (current == end)                               \
    {                                                 \
        return std::pair{current, line_number};       \
    }                                                 \
    if (*current != ';' and *current != '\n')         \
    {                                                 \
        skip_whitespace;                              \
        if (current == end)                           \
        {                                             \
            return std::pair{current, line_number};   \
        }                                             \
    }                                                 \
    if (*current == ';')                              \
    {                                                 \
        do                                            \
        {                                             \
            current++;                                \
            column_number++;                          \
        } while (current < end and *current != '\n'); \
        if (current == end)                           \
        {                                             \
            return std::pair{current, line_number};   \
        }                                             \
    }                                                 \
    if (*current == '\n')                             \
    {                                                 \
        action;                                       \
    }                                                 \
    else                                              \
    {                                                 \
        nonempty;                                     \
    }

#define last_word(type, action) maybe_empty(action, parsing_error("Unexpected character '" << *current << "' after " #type " statement"))

#define last_word_cleanup(type) last_word(type, return (std::pair{current + 1, line_number + 1}))

#define parse_unsigned_index(name)         \
    std::uint32_t name = 0;                \
    do                                     \
    {                                      \
        name = name * 10 + *current - '0'; \
        current++;                         \
        column_number++;                   \
    } while (current < end and std::isdigit(*current))

#define parse_word(string)    \
    do                        \
    {                         \
        string += *current++; \
        column_number++;      \
    } while (current < end and *current != ';' and !std::isspace(*current))

#define dispatch(key, name)                                                                      \
    case keywords::keyword::key:                                                                 \
    {                                                                                            \
        auto error = parse_##name(current, end, ret, line_number, column_number, error_builder); \
        if (std::holds_alternative<std::string>(error))                                          \
        {                                                                                        \
            return std::get<std::string>(error);                                                 \
        }                                                                                        \
        else                                                                                     \
        {                                                                                        \
            std::tie(current, line_number) = std::get<std::pair<char *, std::size_t>>(error);    \
        }                                                                                        \
        break;                                                                                   \
    }
#define parse_helper(type) std::variant<std::pair<char *, std::size_t>, std::string> parse_##type(char *current, char *end, oops_bcode_compiler::parsing::cls &cls, std::size_t line_number, std::size_t column_number, std::stringstream &error_builder)

    parse_helper(import)
    {
        skip_whitespace;
        guard_end;
        if (*current == '#')
        {
            current++;
            guard_end;
            if (std::isspace(*current))
            {
                skip_whitespace;
                guard_end;
            }
            std::string import_name;
            parse_word(import_name);
            guard_end;
            skip_whitespace;
            guard_end;
            std::string keyword_builder;
            parse_word(keyword_builder);
            guard_end;
            skip_whitespace;
            guard_end;
            std::transform(keyword_builder.begin(), keyword_builder.end(), keyword_builder.begin(), [](char c) { return std::toupper(static_cast<unsigned char>(c)); });
            if (auto keyword = oops_bcode_compiler::keywords::string_to_keywords.find(keyword_builder); keyword != oops_bcode_compiler::keywords::string_to_keywords.end())
            {
                std::string *builder_ptr;
                switch (keyword->second)
                {
                case oops_bcode_compiler::keywords::keyword::PROC:
                {
                    cls.methods.push_back({import_name, ""});
                    builder_ptr = &cls.methods.back().name;
                    break;
                }
                case oops_bcode_compiler::keywords::keyword::IVAR:
                {
                    cls.instance_variables.push_back({import_name, ""});
                    builder_ptr = &cls.instance_variables.back().name;
                    break;
                }
                case oops_bcode_compiler::keywords::keyword::SVAR:
                {
                    cls.methods.push_back({import_name, ""});
                    builder_ptr = &cls.static_variables.back().name;
                    break;
                }
                default:
                    parsing_error(keyword_builder << " is not valid within an import statement!");
                }
                skip_whitespace;
                guard_end;
                parse_word(*builder_ptr);
                last_word_cleanup(import);
            }
            else
            {
                parsing_error(keyword_builder << " is not a valid keyword!");
            }
        }
        else
        {
            std::string &builder = cls.imports.emplace_back();
            parse_word(builder);
            last_word_cleanup(import);
        }
    }

    parse_helper(extends)
    {
        if (cls.imports.size() != 7)
        {
            parsing_error("Non-extended class already defined!");
        }
        cls.implement_count++;
        return parse_import(current, end, cls, line_number, column_number, error_builder);
    }
    parse_helper(implements)
    {
        if (cls.imports.size() != 6 + cls.implement_count + 1)
        {
            parsing_error("Non-extended class already defined!");
        }
        cls.implement_count++;
        return parse_import(current, end, cls, line_number, column_number, error_builder);
    }

    parse_helper(instance_variable)
    {
        skip_whitespace;
        guard_end;
        if (!std::isdigit(*current))
        {
            parsing_error("Unexpected non-digit character!");
        }
        std::string import_name;
        parse_word(import_name);
        guard_end;
        if (!std::isspace(*current))
        {
            parsing_error("Unexpected non-digit character!");
        }
        skip_whitespace;
        guard_end;
        cls.self_instances.push_back({import_name, ""});
        parse_word(cls.self_instances.back().name);
        last_word_cleanup(instance_variable);
    }

    parse_helper(static_variable)
    {
        skip_whitespace;
        guard_end;
        if (!std::isdigit(*current))
        {
            parsing_error("Unexpected non-digit character");
        }
        std::string import_name;
        parse_word(import_name);
        guard_end;
        if (!std::isspace(*current))
        {
            parsing_error("Unexpected non-digit character while parsing import index!");
        }
        skip_whitespace;
        guard_end;
        cls.self_statics.push_back({import_name, ""});
        parse_word(cls.self_statics.back().name);
        last_word_cleanup(static_variable);
    }

    parse_helper(class)
    {
        skip_whitespace;
        guard_end;
        if (cls.imports.size() != 6)
        {
            parsing_error("Extra imports before class name!");
        }
        cls.imports.push_back("");
        parse_word(cls.imports.back());
        last_word_cleanup(class);
    }

    std::variant<std::pair<char *, std::size_t>, std::string, int> parse_line(char *current, char *end, oops_bcode_compiler::parsing::cls &cls, std::size_t line_number, std::size_t column_number, std::stringstream &error_builder)
    {
        maybe_empty(return 0;, );
        std::string keyword_builder;
        parse_word(keyword_builder);
        guard_end;
        typedef oops_bcode_compiler::keywords::keyword kw;
        std::transform(keyword_builder.begin(), keyword_builder.end(), keyword_builder.begin(), [](char c) { return std::toupper(static_cast<unsigned char>(c)); });
        if (auto keyword = oops_bcode_compiler::keywords::string_to_keywords.find(keyword_builder); keyword != oops_bcode_compiler::keywords::string_to_keywords.end())
        {
            switch (keyword->second)
            {
            case kw::ADD:
            case kw::ADDI:
            case kw::AND:
            case kw::ANDI:
            case kw::DIV:
            case kw::DIVI:
            case kw::DIVU:
            case kw::DIVUI:
            case kw::MUL:
            case kw::MULI:
            case kw::OR:
            case kw::ORI:
            case kw::SLL:
            case kw::SLLI:
            case kw::SRA:
            case kw::SRAI:
            case kw::SRL:
            case kw::SRLI:
            case kw::SUB:
            case kw::SUBI:
            case kw::XOR:
            case kw::XORI:
            case kw::BEQ:
            case kw::BEQI:
            case kw::BGE:
            case kw::BGEI:
            case kw::BGT:
            case kw::BGTI:
            case kw::BLE:
            case kw::BLEI:
            case kw::BLT:
            case kw::BLTI:
            case kw::BNEQ:
            case kw::BNEQI:
            case kw::BU:
            case kw::VLLD:
            case kw::VLSR:
            case kw::STLD:
            case kw::STSR:
            case kw::ALD:
            case kw::ASR:
            case kw::VINV:
            case kw::SINV:
            case kw::IINV:
            {
                cls.self_methods.back().instructions.push_back({"", "", "", keyword->second});
                auto instruction = cls.self_methods.back().instructions.back();
                skip_whitespace;
                guard_end;
                parse_word(instruction.dest);
                guard_end;
                skip_whitespace;
                guard_end;
                parse_word(instruction.src1);
                guard_end;
                skip_whitespace;
                guard_end;
                parse_word(instruction.src2);
                last_word_cleanup(instruction);
            }
            case kw::LUI:
            case kw::LLI:
            case kw::NEG:
            case kw::ICSTL:
            case kw::ICSTF:
            case kw::ICSTD:
            case kw::LCSTI:
            case kw::LCSTF:
            case kw::LCSTD:
            case kw::FCSTI:
            case kw::FCSTL:
            case kw::FCSTD:
            case kw::DCSTI:
            case kw::DCSTL:
            case kw::DCSTF:
            case kw::CANEW:
            case kw::SANEW:
            case kw::IANEW:
            case kw::LANEW:
            case kw::FANEW:
            case kw::DANEW:
            case kw::VANEW:
            case kw::DEF:
            case kw::VNEW:
            case kw::IOF:
            {
                cls.self_methods.back().instructions.push_back({"", "", "", keyword->second});
                auto instruction = cls.self_methods.back().instructions.back();
                skip_whitespace;
                guard_end;
                parse_word(instruction.dest);
                guard_end;
                skip_whitespace;
                guard_end;
                parse_word(instruction.src1);
                last_word_cleanup(instruction);
            }
            case kw::LNL:
            case kw::RET:
            case kw::PASS:
            {
                cls.self_methods.back().instructions.push_back({"", "", "", keyword->second});
                auto instruction = cls.self_methods.back().instructions.back();
                skip_whitespace;
                guard_end;
                parse_word(instruction.dest);
                last_word_cleanup(instruction);
            }
            case kw::ARG:
            {
                skip_whitespace;
                guard_end;
                std::string import_name;
                parse_word(import_name);
                guard_end;
                if (!std::isspace(*current))
                {
                    parsing_error("Unexpected non-digit character while parsing import index!");
                }
                skip_whitespace;
                guard_end;
                cls.self_methods.back().parameters.push_back({import_name, ""});
                parse_word(cls.self_methods.back().parameters.back().name);
                last_word_cleanup(instruction);
            }
            default:
                parsing_error(keyword_builder << " is not a valid instruction within a method!");
            }
        }
        else
        {
            parsing_error(keyword_builder << " is not a valid keyword!");
        }
    }

    parse_helper(procedure)
    {
        skip_whitespace;
        guard_end;
        std::string return_class_name;
        parse_word(return_class_name);
        guard_end;
        skip_whitespace;
        cls.methods.push_back({cls.imports[6], ""});
        parse_word(cls.methods.back().name);
        last_word(procedure, current++);
        cls.self_methods.push_back({return_class_name, {}, {}});
        std::variant<std::pair<char *, std::size_t>, std::string, int> next = std::pair{current, line_number + 1};
        do
        {
            std::tie(current, line_number) = std::get<std::pair<char *, std::size_t>>(next);
            next = parse_line(current, end, cls, line_number, column_number, error_builder);
        } while (std::holds_alternative<std::pair<char *, std::size_t>>(next));
        if (std::holds_alternative<std::string>(next))
        {
            return std::get<std::string>(next);
        }
        else
        {
            do
                current++;
            while (current < end and *current != '\n');
            if (current == end)
            {
                return std::pair{current, line_number};
            }
            return std::pair{current + 1, line_number + 1};
        }
    }

    parse_helper(static_procedure)
    {
        cls.static_method_count++;
        return parse_procedure(current, end, cls, line_number, column_number, error_builder);
    }
#undef parse_helper
} // namespace

std::variant<cls, std::string> parser::parse()
{
    if (!this->mapping.mmapped_file)
    {
        return "File was not opened!";
    }
    cls ret;
    for (auto imp : {"char", "short", "int", "long", "float", "double"})
    {
        ret.imports.emplace_back(imp);
    }
    ret.implement_count = ret.static_method_count = 0;
    std::stringstream error_builder;
    std::string keyword_builder;
    char *current = this->mapping.mmapped_file, *end = current + this->mapping.file_size;
    std::size_t line_number = 0, column_number = 0;
    while (current < end)
    {
        if (std::isspace(*current))
        {
            skip_whitespace;
            guard_end;
        }
        parse_word(keyword_builder);
        guard_end;
        std::transform(keyword_builder.begin(), keyword_builder.end(), keyword_builder.begin(), [](char c) { return std::toupper(static_cast<unsigned char>(c)); });
        if (auto keyword = keywords::string_to_keywords.find(keyword_builder); keyword != keywords::string_to_keywords.end())
        {
            switch (keyword->second)
            {
                dispatch(CLZ, class);
                dispatch(IMP, import);
                dispatch(IVAR, instance_variable);
                dispatch(SVAR, static_variable);
                dispatch(PROC, procedure);
                dispatch(SPROC, static_procedure);
                dispatch(EXT, extends);
                dispatch(IMPL, implements);
            default:
                parsing_error("Keyword '" << keyword_builder << "' is not a class-level keyword!");
            }
        }
        else
        {
            parsing_error("Unknown keyword '" << keyword_builder << "'");
        }
    }
    return ret;
}