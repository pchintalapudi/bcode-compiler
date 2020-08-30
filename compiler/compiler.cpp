#include "compiler.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>

#include "../instructions/keywords.h"
#include "../utils/hashing.h"
#include "../utils/puns.h"
#include "../debug/logs.h"

using namespace oops_bcode_compiler::compiler;
using namespace oops_bcode_compiler::debug;

namespace
{
    struct var
    {
        std::uint16_t offset;
        std::uint8_t type;
    };
    enum class itype : unsigned char
    {
#pragma region DO NOT UNFOLD ME
        //Done
        NOP,
        IADD,
        LADD,
        FADD,
        DADD,
        ISUB,
        LSUB,
        FSUB,
        DSUB,
        IMUL,
        LMUL,
        FMUL,
        DMUL,
        IDIV,
        LDIV,
        FDIV,
        DDIV,
        IDIVU,
        LDIVU,
        IADDI,
        LADDI,
        FADDI,
        DADDI,
        ISUBI,
        LSUBI,
        FSUBI,
        DSUBI,
        IMULI,
        LMULI,
        FMULI,
        DMULI,
        IDIVI,
        LDIVI,
        FDIVI,
        DDIVI,
        IDIVUI,
        LDIVUI,
        //TODO
        INEG,
        LNEG,
        FNEG,
        DNEG,
        LUI,
        LDI,
        LNL,
        ICSTL,
        ICSTF,
        ICSTD,
        LCSTI,
        LCSTF,
        LCSTD,
        FCSTI,
        FCSTL,
        FCSTD,
        DCSTI,
        DCSTL,
        DCSTF,
        //Done
        IAND,
        LAND,
        IOR,
        LOR,
        IXOR,
        LXOR,
        ISLL,
        LSLL,
        ISRL,
        LSRL,
        ISRA,
        LSRA,
        IANDI,
        LANDI,
        IORI,
        LORI,
        IXORI,
        LXORI,
        ISLLI,
        LSLLI,
        ISRLI,
        LSRLI,
        ISRAI,
        LSRAI,
        IBGE,
        LBGE,
        FBGE,
        DBGE,
        IBLT,
        LBLT,
        FBLT,
        DBLT,
        IBLE,
        LBLE,
        FBLE,
        DBLE,
        IBGT,
        LBGT,
        FBGT,
        DBGT,
        IBEQ,
        LBEQ,
        FBEQ,
        DBEQ,
        VBEQ,
        IBNEQ,
        LBNEQ,
        FBNEQ,
        DBNEQ,
        VBNEQ,
        IBGEI,
        LBGEI,
        FBGEI,
        DBGEI,
        IBLTI,
        LBLTI,
        FBLTI,
        DBLTI,
        IBLEI,
        LBLEI,
        FBLEI,
        DBLEI,
        IBGTI,
        LBGTI,
        FBGTI,
        DBGTI,
        IBEQI,
        LBEQI,
        FBEQI,
        DBEQI,
        VBEQI,
        IBNEQI,
        LBNEQI,
        FBNEQI,
        DBNEQI,
        VBNEQI,
        //TODO
        IBCMP,
        LBCMP,
        FBCMP,
        DBCMP,
        BADR,
        BU,
        CVLLD,
        SVLLD,
        IVLLD,
        LVLLD,
        FVLLD,
        DVLLD,
        VVLLD,
        CVLSR,
        SVLSR,
        IVLSR,
        LVLSR,
        FVLSR,
        DVLSR,
        VVLSR,
        CALD,
        SALD,
        IALD,
        LALD,
        FALD,
        DALD,
        VALD,
        CASR,
        SASR,
        IASR,
        LASR,
        FASR,
        DASR,
        VASR,
        CSTLD,
        SSTLD,
        ISTLD,
        LSTLD,
        FSTLD,
        DSTLD,
        VSTLD,
        CSTSR,
        SSTSR,
        ISTSR,
        LSTSR,
        FSTSR,
        DSTSR,
        VSTSR,
        VNEW,
        //Done
        CANEW,
        SANEW,
        IANEW,
        LANEW,
        FANEW,
        DANEW,
        VANEW,
        //TODO
        IOF,
        VINV,
        SINV,
        IINV,
        IRET,
        LRET,
        FRET,
        DRET,
        VRET,
        EXC
#pragma endregion
    };

    std::uint64_t construct3(itype type, std::uint8_t flags, std::uint16_t dest, std::uint16_t src1, std::uint16_t src2)
    {
        std::uint64_t out = 0;
        out <<= CHAR_BIT * sizeof(type);
        out |= static_cast<std::uint8_t>(type);
        out <<= CHAR_BIT * sizeof(flags);
        out |= flags;
        out <<= CHAR_BIT * sizeof(src2);
        out |= src2;
        out <<= CHAR_BIT * sizeof(src1);
        out |= src1;
        out <<= CHAR_BIT * sizeof(dest);
        out |= dest;
        return out;
    }
    std::uint64_t construct24(itype type, std::uint16_t dest, std::uint16_t src1, std::uint32_t imm24)
    {
        std::uint64_t out = 0;
        out <<= CHAR_BIT * sizeof(type);
        out |= static_cast<std::uint8_t>(type);
        out <<= CHAR_BIT * (sizeof(imm24) - sizeof(type));
        out |= imm24;
        out <<= sizeof(src1);
        out |= src1;
        out <<= sizeof(dest);
        out |= dest;
        return out;
    }
    std::uint64_t construct32(itype type, std::uint8_t flags, std::uint16_t dest, std::uint32_t imm32)
    {
        std::uint64_t out = 0;
        out <<= CHAR_BIT * sizeof(type);
        out |= static_cast<std::uint8_t>(type);
        out <<= CHAR_BIT * sizeof(flags);
        out |= static_cast<std::uint8_t>(flags);
        out <<= CHAR_BIT * sizeof(imm32);
        out |= imm32;
        out <<= CHAR_BIT * sizeof(dest);
        out |= dest;
        return out;
    }
    std::uint64_t construct40(itype type, std::uint16_t dest, std::uint64_t imm40)
    {
        imm40 |= static_cast<std::uint64_t>(type) << 40;
        imm40 <<= CHAR_BIT * sizeof(dest);
        imm40 |= dest;
        return imm40;
    }

    std::variant<std::int32_t, std::string> parse_int(const std::string &str)
    {
#define pfail return "'" + str + "' could not be parsed as an integer"
        std::size_t counted = ~static_cast<std::size_t>(0);
        std::int32_t parsed;
        try
        {
            if (str.length() > 2 and str[0] == '0')
            {
                switch (str[1])
                {
                case 'b':
                case 'B':
                {
                    parsed = std::stoi(str.substr(2), &counted, 2);
                    break;
                }
                case 'o':
                case 'O':
                {
                    parsed = std::stoi(str.substr(2), &counted, 8);
                    break;
                }
                case 'x':
                case 'X':
                {
                    parsed = std::stoi(str.substr(2), &counted, 16);
                    break;
                }
                default:
                    break;
                }
                if (counted != str.length() - 2)
                {
                    pfail;
                }
            }
            else
            {
                parsed = std::stoi(str, &counted, 10);
                if (counted != str.length())
                {
                    pfail;
                }
            }
        }
        catch (std::invalid_argument &)
        {
            pfail;
        }
        catch (std::out_of_range &)
        {
            return "'" + str + "' is too large to be an integer";
        }
        return parsed;
#undef pfail
    }
    std::variant<std::int64_t, std::string> parse_long(const std::string &str)
    {
#define pfail return "'" + str + "' could not be parsed as a long"
        std::size_t counted = ~static_cast<std::size_t>(0);
        std::int64_t parsed;
        try
        {
            if (str.length() > 2 and str[0] == '0')
            {
                switch (str[1])
                {
                case 'b':
                case 'B':
                {
                    parsed = std::stoll(str.substr(2), &counted, 2);
                    break;
                }
                case 'o':
                case 'O':
                {
                    parsed = std::stoll(str.substr(2), &counted, 8);
                    break;
                }
                case 'x':
                case 'X':
                {
                    parsed = std::stoll(str.substr(2), &counted, 16);
                    break;
                }
                default:
                    break;
                }
                if (counted != str.length() - 2)
                {
                    pfail;
                }
            }
            else
            {
                parsed = std::stoll(str, &counted, 10);
                if (counted != str.length())
                {
                    pfail;
                }
            }
        }
        catch (std::invalid_argument &)
        {
            pfail;
        }
        catch (std::out_of_range &)
        {
            return "'" + str + "' is too large to be a long";
        }
        return parsed;
#undef pfail
    }

    std::variant<float, std::string> parse_float(const std::string &str)
    {
        std::size_t counted;
        try
        {
            float parsed = std::stof(str, &counted);
            if (counted != str.length())
            {
                return "'" + str + "' could not be parsed as a float";
            }
            return parsed;
        }
        catch (std::invalid_argument &)
        {
            return "'" + str + "' could not be parsed as a float";
        }
        catch (std::out_of_range &)
        {
            return "'" + str + "' is too large to be a float";
        }
    }

    std::variant<double, std::string> parse_double(const std::string &str)
    {
        std::size_t counted;
        try
        {
            double parsed = std::stod(str, &counted);
            if (counted != str.length())
            {
                return "'" + str + "' could not be parsed as a double";
            }
            return parsed;
        }
        catch (std::invalid_argument &)
        {
            return "'" + str + "' could not be parsed as a double";
        }
        catch (std::out_of_range &)
        {
            return "'" + str + "' is too large to be a double";
        }
    }

    std::variant<std::int32_t, std::string> to24(const std::string &str)
    {
        auto parsed = parse_int(str);
        if (std::holds_alternative<std::int32_t>(parsed))
        {
            if (static_cast<std::uint32_t>(std::abs(std::get<std::int32_t>(parsed))) >> 23)
            {
                return "'" + str + "' is too large to be a 24-bit integer";
            }
            return static_cast<std::int32_t>(static_cast<std::uint32_t>(std::get<std::int32_t>(parsed)) << (32 - 24) >> (32 - 24));
        }
        return parsed;
    }
    std::variant<std::int16_t, std::string> to16(const std::string &str)
    {
        auto parsed = parse_int(str);
        if (std::holds_alternative<std::int32_t>(parsed))
        {
            if (static_cast<std::uint32_t>(std::abs(std::get<std::int32_t>(parsed))) >> 15)
            {
                return "'" + str + "' is too large to be a 16-bit integer";
            }
            return static_cast<std::int16_t>(std::get<std::int32_t>(parsed));
        }
        return std::get<std::string>(parsed);
    }

    constexpr std::uint8_t cast_types(std::uint8_t src, std::uint8_t dest)
    {
        return src * 16 + dest;
    }

    std::size_t round_off(std::size_t in, std::size_t align)
    {
        return (in + align - 1) & ~(align - 1);
    }
} // namespace

constexpr static std::uint8_t static_method_type = 5, virtual_method_type = 4;

std::variant<method, std::vector<std::string>> oops_bcode_compiler::compiler::compile(oops_bcode_compiler::parsing::cls::procedure &proc)
{
    static const std::unordered_map<std::string, std::uint8_t> type_map = {{"int", 2}, {"long", 3}, {"float", 4}, {"double", 5}, {"ref", 6}};
    std::vector<std::string> errors;
#define compile_error(error, line, col)                                                       \
    std::stringstream error_builder;                                                          \
    error_builder << error << " at line " << line << ", column " << col;                      \
    logger.builder(logging::level::debug) << error_builder.str() << logging::logbuilder::end; \
    errors.push_back(error_builder.str())
    method mtd;
    mtd.name = proc.name;
    mtd.method_type = proc.is_static ? static_method_type : virtual_method_type;
    if (auto type = type_map.find(proc.return_type_name); type != type_map.end())
    {
        mtd.return_type = type->second;
    }
    else
    {
        compile_error("Invalid return type " << proc.return_type_name, proc.line_number, proc.column_number);
    }
    std::unordered_map<std::string, var> local_variables;
    for (auto &param : proc.parameters)
    {
        if (local_variables.find(param.name) != local_variables.end())
        {
            compile_error("Local variable " << param.name << " was redefined with type " << param.host_name, param.line_number, param.column_number);
            continue;
        }
        if (auto type = type_map.find(param.host_name); type != type_map.end())
        {
            local_variables[param.name] = {mtd.stack_size, type->second};
            mtd.arg_types.push_back(type->second);
            switch (type->second)
            {
            case 2:
                mtd.stack_size += sizeof(std::int32_t) / sizeof(std::int32_t);
                break;
            case 3:
                mtd.stack_size += sizeof(std::int64_t) / sizeof(std::int32_t);
                break;
            case 4:
                mtd.stack_size += sizeof(float) / sizeof(std::int32_t);
                break;
            case 5:
                mtd.stack_size += sizeof(double) / sizeof(std::int32_t);
                break;
            case 6:
            {
                compile_error("Method argument " << param.name << " must have real type, not ref", param.host_name, param.line_number);
                continue;
            }
            }
        }
        else
        {
            mtd.arg_types.push_back(6);
            mtd.handle_map.push_back(mtd.stack_size);
            local_variables[param.name] = {mtd.stack_size, 6};
            mtd.stack_size += sizeof(char *) / sizeof(std::int32_t);
        }
    }
    std::unordered_map<std::string, std::uint16_t> labels;
#pragma region

#define lookup_variable(name, off)                                                                                                                                                                                                                                                                                                                                                          \
    auto name##_it = local_variables.find(instr.operands[off]);                                                                                                                                                                                                                                                                                                                             \
    if (name##_it == local_variables.end())                                                                                                                                                                                                                                                                                                                                                 \
    {                                                                                                                                                                                                                                                                                                                                                                                       \
        compile_error("Undefined local variable " << instr.operands[off], instr.line_number, instr.column_number);                                                                                                                                                                                                                                                                          \
        continue;                                                                                                                                                                                                                                                                                                                                                                           \
    }                                                                                                                                                                                                                                                                                                                                                                                       \
    logger.builder(logging::level::debug) << "Stack offset of " #name " " << instr.operands[off] << " (argument " << std::to_string(static_cast<int>(off)) << ") is " << name##_it->second.offset << " and has type " << name##_it->second.type << " (Source line & col " << instr.line_number << ", " << instr.column_number << "), called from " << __LINE__ << logging::logbuilder::end; \
    auto name = name##_it->second
#pragma endregion
    unsigned instr_count;
    typedef keywords::keyword ktype;
    for (auto &instr : proc.instructions)
    {
        switch (instr.itype)
        {
        case ktype::DEF:
        {
            if (local_variables.find(instr.operands[1]) != local_variables.end())
            {
                compile_error("Redefining local variable " << instr.operands[1], instr.line_number, instr.column_number);
                continue;
            }
            if (auto type = type_map.find(instr.operands[0]); type != type_map.end())
            {
                local_variables[instr.operands[1]] = {mtd.stack_size, type->second};
                switch (type->second)
                {
                case 2:
                    mtd.stack_size += sizeof(std::int32_t) / sizeof(std::int32_t);
                    break;
                case 3:
                    mtd.stack_size += sizeof(std::int64_t) / sizeof(std::int32_t);
                    break;
                case 4:
                    mtd.stack_size += sizeof(float) / sizeof(std::int32_t);
                    break;
                case 5:
                    mtd.stack_size += sizeof(double) / sizeof(std::int32_t);
                    break;
                case 6:
                {
                    mtd.handle_map.push_back(mtd.stack_size);
                    mtd.stack_size += sizeof(char *) / sizeof(std::int32_t);
                    break;
                }
                }
            }
            else
            {
                compile_error("Invalid type " << instr.operands[0], instr.line_number, instr.column_number);
            }
            break;
        }
        case ktype::LBL:
        {
            if (labels.find(instr.operands[0]) != labels.end())
            {
                compile_error("Redefining label " << instr.operands[0], instr.line_number, instr.column_number);
                continue;
            }
            labels[instr.operands[0]] = instr_count;
            break;
        }
        case ktype::SINV:
        {
            instr_count += 1 + (instr.operands.size() - 1 + sizeof(std::uint64_t) / sizeof(std::uint16_t) - 1) / (sizeof(std::uint64_t) / sizeof(std::uint16_t));
            break;
        }
        case ktype::IINV:
        case ktype::VINV:
        {
            instr_count += 1 + (instr.operands.size() - 2 + sizeof(std::uint64_t) / sizeof(std::uint16_t) - 1) / (sizeof(std::uint64_t) / sizeof(std::uint16_t));
            break;
        }
        case ktype::LI:
        {
            lookup_variable(dest, 0);
            switch (dest.type)
            {
            case 2:
            {
                if (instr.operands[1][0] == '\'')
                {
                    if (instr.operands[1].length() < 2 or instr.operands[1].back() != '\'')
                    {
                        compile_error("Unclosed character literal", instr.line_number, instr.column_number);
                        continue;
                    }
                    std::uint32_t imm = 0;
                    unsigned byte_count = 0;
                    bool fail = false;
                    for (std::size_t i = 1; i < instr.operands[1].length() - 1; i++)
                    {
                        if (byte_count == 4)
                        {
                            compile_error("Character literal too large", instr.line_number, instr.column_number);
                            fail = true;
                            break;
                        }
                        unsigned char c = instr.operands[1][i];
                        if (c == '\\')
                        {
                            i++;
                            if (i == instr.operands[1].length() - 1)
                            {
                                compile_error("Closing character literal ' was escaped", instr.line_number, instr.column_number);
                                fail = true;
                                break;
                            }
                            switch (instr.operands[1][i])
                            {
                            case 'a':
                                c = '\a';
                                break;
                            case 'b':
                                c = '\b';
                                break;
                            case 'e':
                                c = '\e';
                                break;
                            case 'f':
                                c = '\f';
                                break;
                            case 'n':
                                c = '\n';
                                break;
                            case 'r':
                                c = '\r';
                                break;
                            case 's':
                                c = ' ';
                                break;
                            case 't':
                                c = '\t';
                                break;
                            case 'v':
                                c = '\v';
                                break;
                            case '\\':
                                c = '\\';
                                break;
                            case '\'':
                                c = '\'';
                                break;
                            case '"':
                                c = '"';
                                break;
                            case '\?':
                                c = '\?';
                                break;
                            default:
                            {
                                compile_error("Invalid escape sequence \\" << static_cast<char>(c) << " in character literal", instr.line_number, instr.column_number);
                                fail = true;
                                break;
                            }
                            }
                        }
                        imm <<= CHAR_BIT * sizeof(unsigned char);
                        imm |= c;
                        byte_count++;
                    }
                    if (fail)
                    {
                        continue;
                    }
                    std::string out;
                    out.reserve(sizeof(std::int32_t));
                    utils::pun_write(&out[0], imm);
                    instr.operands[1] = out;
                    instr_count++;
                    break;
                }
                else
                {
                    auto parsed = ::parse_int(instr.operands[1]);
                    if (std::holds_alternative<std::string>(parsed))
                    {
                        compile_error("Error compiling int immediate: " << std::get<std::string>(parsed), instr.line_number, instr.column_number);
                        continue;
                    }
                    else
                    {
                        std::string out;
                        out.reserve(sizeof(std::int32_t));
                        utils::pun_write(&out[0], std::get<std::int32_t>(parsed));
                        instr.operands[1] = out;
                    }
                }
                instr_count++;
                break;
            }
            case 3:
            {
                auto parsed = ::parse_long(instr.operands[1]);
                if (std::holds_alternative<std::string>(parsed))
                {
                    compile_error("Error compiling long immediate: " << std::get<std::string>(parsed), instr.line_number, instr.column_number);
                    continue;
                }
                else
                {
                    std::string out;
                    out.reserve(sizeof(std::int64_t));
                    utils::pun_write(&out[0], std::get<std::int64_t>(parsed));
                    instr.operands[1] = out;
                    instr_count += 1 + (std::get<std::int64_t>(parsed) << (sizeof(std::uint64_t) - sizeof(std::uint16_t) - sizeof(std::uint8_t)) * CHAR_BIT);
                }
                break;
            }
            case 4:
            {
                auto parsed = ::parse_float(instr.operands[2]);
                if (std::holds_alternative<std::string>(parsed))
                {
                    compile_error("Error compiling float immediate: " << std::get<std::string>(parsed), instr.line_number, instr.column_number);
                    continue;
                }
                else
                {
                    std::string out;
                    out.reserve(sizeof(float));
                    utils::pun_write(&out[0], std::get<float>(parsed));
                    instr.operands[1] = out;
                }
                instr_count++;
                break;
            }
            case 5:
            {
                auto parsed = ::parse_double(instr.operands[2]);
                if (std::holds_alternative<std::string>(parsed))
                {
                    compile_error("Error compiling double immediate: " << std::get<std::string>(parsed), instr.line_number, instr.column_number);
                    continue;
                }
                else
                {
                    std::string out;
                    out.reserve(sizeof(double));
                    utils::pun_write(&out[0], std::get<double>(parsed));
                    instr.operands[1] = out;
                    instr_count += 1 + (utils::pun_reinterpret<std::uint64_t>(std::get<double>(parsed)) << (sizeof(double) - sizeof(std::uint16_t) - sizeof(std::uint8_t)) * CHAR_BIT);
                }
                instr_count++;
                break;
            }
            case 6:
            {
                if (instr.operands[1] != "null")
                {
                    compile_error("Cannot load non-null immediate for object", instr.line_number, instr.column_number);
                    continue;
                }
                instr_count++;
                break;
            }
            }
            break;
        }
        default:
            instr_count++;
            break;
        }
    }
#pragma region

#define match_types(var1, var2)                                                                                                                                      \
    if (var1.type != var2.type)                                                                                                                                      \
    {                                                                                                                                                                \
        compile_error("Expected " #var1 " (" << var1.type << ") and " #var2 " (" << var2.type << ") to have the same type", instr.line_number, instr.column_number); \
    }
#pragma endregion
    for (auto &instr : proc.instructions)
    {
        logger.builder(logging::level::debug) << "Instruction " << keywords::keyword_to_string[static_cast<unsigned>(instr.itype)] << logging::logbuilder::end;
        for (auto &operand : instr.operands)
        {
            logger.builder(logging::level::debug) << "Operand " << operand << logging::logbuilder::end;
        }
        switch (instr.itype)
        {
#pragma region

#define pinst(type, letter, keyword)                                                                                  \
    case type:                                                                                                        \
        mtd.instructions.push_back(::construct3(::itype::letter##keyword, 0, dest.offset, src1.offset, src2.offset)); \
        break
#pragma endregion
#pragma region

#define pkey(keyword)                                                                                                                               \
    case ktype::keyword:                                                                                                                            \
    {                                                                                                                                               \
        lookup_variable(dest, 0);                                                                                                                   \
        lookup_variable(src1, 1);                                                                                                                   \
        lookup_variable(src2, 2);                                                                                                                   \
        match_types(dest, src1);                                                                                                                    \
        match_types(src1, src2);                                                                                                                    \
        switch (dest.type)                                                                                                                          \
        {                                                                                                                                           \
            pinst(2, I, keyword);                                                                                                                   \
            pinst(3, L, keyword);                                                                                                                   \
            pinst(4, F, keyword);                                                                                                                   \
            pinst(5, D, keyword);                                                                                                                   \
        default:                                                                                                                                    \
        {                                                                                                                                           \
            compile_error("Operands of type " << dest.type << " cannot be used for instruction " #keyword, instr.line_number, instr.column_number); \
            continue;                                                                                                                               \
        }                                                                                                                                           \
        }                                                                                                                                           \
        break;                                                                                                                                      \
    }
#pragma endregion
            pkey(ADD);
            pkey(SUB);
            pkey(MUL);
            pkey(DIV);
#pragma region

#define lookup_imm24                                                                                                            \
    auto imm24_var = ::to24(instr.operands[2]);                                                                                 \
    if (std::holds_alternative<std::string>(imm24_var))                                                                         \
    {                                                                                                                           \
        compile_error("Error parsing immediate: " << std::get<std::string>(imm24_var), instr.line_number, instr.column_number); \
        continue;                                                                                                               \
    }                                                                                                                           \
    std::int32_t imm24 = std::get<std::int32_t>(imm24_var)
#pragma endregion
#pragma region

#define pinsti(type, letter, keyword)                                                                         \
    case type:                                                                                                \
        mtd.instructions.push_back(::construct24(::itype::letter##keyword, dest.offset, src1.offset, imm24)); \
        break
#pragma endregion
#pragma region
#define pkeyi(keyword)                                                                                                                              \
    case ktype::keyword:                                                                                                                            \
    {                                                                                                                                               \
        lookup_variable(dest, 0);                                                                                                                   \
        lookup_variable(src1, 1);                                                                                                                   \
        match_types(dest, src1);                                                                                                                    \
        lookup_imm24;                                                                                                                               \
        switch (dest.type)                                                                                                                          \
        {                                                                                                                                           \
            pinsti(2, I, keyword);                                                                                                                  \
            pinsti(3, L, keyword);                                                                                                                  \
            pinsti(4, F, keyword);                                                                                                                  \
            pinsti(5, D, keyword);                                                                                                                  \
        default:                                                                                                                                    \
        {                                                                                                                                           \
            compile_error("Operands of type " << dest.type << " cannot be used for instruction " #keyword, instr.line_number, instr.column_number); \
            continue;                                                                                                                               \
        }                                                                                                                                           \
        }                                                                                                                                           \
        break;                                                                                                                                      \
    }
#pragma endregion
            pkeyi(ADDI);
            pkeyi(SUBI);
            pkeyi(MULI);
            pkeyi(DIVI);
#pragma region

#define ikey(keyword)                                                                                                                               \
    case ktype::keyword:                                                                                                                            \
    {                                                                                                                                               \
        lookup_variable(dest, 0);                                                                                                                   \
        lookup_variable(src1, 1);                                                                                                                   \
        lookup_variable(src2, 2);                                                                                                                   \
        match_types(dest, src1);                                                                                                                    \
        match_types(src1, src2);                                                                                                                    \
        switch (dest.type)                                                                                                                          \
        {                                                                                                                                           \
            pinst(2, I, keyword);                                                                                                                   \
            pinst(3, L, keyword);                                                                                                                   \
        default:                                                                                                                                    \
        {                                                                                                                                           \
            compile_error("Operands of type " << dest.type << " cannot be used for instruction " #keyword, instr.line_number, instr.column_number); \
            continue;                                                                                                                               \
        }                                                                                                                                           \
        }                                                                                                                                           \
        break;                                                                                                                                      \
    }
#pragma endregion
            ikey(DIVU);
            ikey(AND);
            ikey(OR);
            ikey(XOR);
            ikey(SLL);
            ikey(SRL);
            ikey(SRA);
#pragma region

#define ikeyi(keyword)                                                                                                                              \
    case ktype::keyword:                                                                                                                            \
    {                                                                                                                                               \
        lookup_variable(dest, 0);                                                                                                                   \
        lookup_variable(src1, 1);                                                                                                                   \
        match_types(dest, src1);                                                                                                                    \
        lookup_imm24;                                                                                                                               \
        switch (dest.type)                                                                                                                          \
        {                                                                                                                                           \
            pinsti(2, I, keyword);                                                                                                                  \
            pinsti(3, L, keyword);                                                                                                                  \
        default:                                                                                                                                    \
        {                                                                                                                                           \
            compile_error("Operands of type " << dest.type << " cannot be used for instruction " #keyword, instr.line_number, instr.column_number); \
            continue;                                                                                                                               \
        }                                                                                                                                           \
        }                                                                                                                                           \
        break;                                                                                                                                      \
    }
#pragma endregion
            ikeyi(DIVUI);
            ikeyi(ANDI);
            ikeyi(ORI);
            ikeyi(XORI);
            ikeyi(SLLI);
            ikeyi(SRLI);
            ikeyi(SRAI);
        case ktype::NEG:
        {
            lookup_variable(dest, 0);
            lookup_variable(src1, 1);
            match_types(dest, src1);
            switch (dest.type)
            {
            case 2:
                mtd.instructions.push_back(::construct3(::itype::INEG, 0, dest.offset, src1.offset, 0));
                break;
            case 3:
                mtd.instructions.push_back(::construct3(::itype::LNEG, 0, dest.offset, src1.offset, 0));
                break;
            case 4:
                mtd.instructions.push_back(::construct3(::itype::FNEG, 0, dest.offset, src1.offset, 0));
                break;
            case 5:
                mtd.instructions.push_back(::construct3(::itype::DNEG, 0, dest.offset, src1.offset, 0));
                break;
            default:
            {
                compile_error("Objects cannot be negated!", instr.line_number, instr.column_number);
                continue;
            }
            }
            break;
        }
        case ktype::LI:
        {
            lookup_variable(dest, 0);
            switch (dest.type)
            {
            case 2:
            case 4:
                mtd.instructions.push_back(::construct32(::itype::LDI, 0, dest.offset, utils::pun_read<std::int32_t>(instr.operands[1].c_str())));
                break;
            case 3:
            case 5:
            {
                std::uint64_t imm = utils::pun_read<std::int64_t>(instr.operands[1].c_str());
                mtd.instructions.push_back(::construct40(::itype::LUI, dest.offset, imm >> (sizeof(std::uint64_t) - sizeof(std::uint16_t) - sizeof(std::uint8_t)) * CHAR_BIT));
                imm <<= (sizeof(std::uint64_t) - sizeof(std::uint16_t) - sizeof(std::uint8_t)) * CHAR_BIT;
                if (imm)
                {
                    mtd.instructions.push_back(::construct24(::itype::LADDI, dest.offset, dest.offset, imm >> (sizeof(std::uint64_t) - sizeof(std::uint16_t) - sizeof(std::uint8_t)) * CHAR_BIT));
                }
                break;
            }
            case 6:
                mtd.instructions.push_back(::construct40(::itype::LNL, dest.offset, 0));
                break;
            }
            break;
        }
        case ktype::CST:
        {
            lookup_variable(dest, 0);
            lookup_variable(src1, 1);
            if (dest.type == src1.type)
            {
                compile_error("Cannot cast between variables " << instr.operands[0] << " and " << instr.operands[1] << " of the same type " << dest.type, instr.line_number, instr.column_number);
                continue;
            }
#define minicast(dtype, dletter, c1, c2, c3, l1, l2, l3)                                                         \
    case dtype:                                                                                                  \
        switch (src1.type)                                                                                       \
        {                                                                                                        \
        case c1:                                                                                                 \
            mtd.instructions.push_back(::construct3(::itype::dletter##CST##l1, 0, dest.offset, src1.offset, 0)); \
            break;                                                                                               \
        case c2:                                                                                                 \
            mtd.instructions.push_back(::construct3(::itype::dletter##CST##l2, 0, dest.offset, src1.offset, 0)); \
            break;                                                                                               \
        case c3:                                                                                                 \
            mtd.instructions.push_back(::construct3(::itype::dletter##CST##l3, 0, dest.offset, src1.offset, 0)); \
            break;                                                                                               \
        }                                                                                                        \
        break
            switch (dest.type)
            {
                minicast(2, I, 3, 4, 5, L, F, D);
                minicast(3, L, 2, 4, 5, I, F, D);
                minicast(4, F, 2, 3, 5, I, L, D);
                minicast(5, D, 2, 3, 4, I, L, F);
            }
            break;
        }
#define lookup_label                                                                                    \
    auto dest##_it = labels.find(instr.operands[0]);                                                    \
    if (dest##_it == labels.end())                                                                      \
    {                                                                                                   \
        compile_error("Undefined label " << instr.operands[0], instr.line_number, instr.column_number); \
        continue;                                                                                       \
    }                                                                                                   \
    auto dest = static_cast<std::make_signed_t<std::size_t>>(static_cast<std::uint16_t>(mtd.instructions.size())) + 1 - dest##_it->second
#define binst(type, letter, keyword)                                                                                            \
    case type:                                                                                                                  \
        mtd.instructions.push_back(::construct3(::itype::letter##keyword, dest > 0, std::abs(dest), src1.offset, src2.offset)); \
        break
#define beop(keyword)                                                                                                                                                                   \
    case ktype::keyword:                                                                                                                                                                \
    {                                                                                                                                                                                   \
        lookup_variable(src1, 1);                                                                                                                                                       \
        lookup_variable(src2, 2);                                                                                                                                                       \
        match_types(src1, src2);                                                                                                                                                        \
        lookup_label;                                                                                                                                                                   \
        mtd.instructions.push_back(::construct3(static_cast<::itype>(static_cast<unsigned>(::itype::I##keyword) + src1.type - 2), dest > 0, std::abs(dest), src1.offset, src2.offset)); \
                                                                                                                                                                                        \
        break;                                                                                                                                                                          \
    }
            beop(BEQ);
            beop(BNEQ);
#define bop(keyword)                                                                                                                                                                                           \
    case ktype::keyword:                                                                                                                                                                                       \
    {                                                                                                                                                                                                          \
        lookup_variable(src1, 1);                                                                                                                                                                              \
        lookup_variable(src2, 2);                                                                                                                                                                              \
        match_types(src1, src2);                                                                                                                                                                               \
        lookup_label;                                                                                                                                                                                          \
        switch (src1.type)                                                                                                                                                                                     \
        {                                                                                                                                                                                                      \
            binst(2, I, keyword);                                                                                                                                                                              \
            binst(3, L, keyword);                                                                                                                                                                              \
            binst(4, F, keyword);                                                                                                                                                                              \
            binst(5, D, keyword);                                                                                                                                                                              \
        default:                                                                                                                                                                                               \
        {                                                                                                                                                                                                      \
            compile_error("Cannot perform comparison operation " << keywords::keyword_to_string[static_cast<unsigned>(instr.itype)] << " on operands of object type", instr.line_number, instr.column_number); \
            continue;                                                                                                                                                                                          \
        }                                                                                                                                                                                                      \
        }                                                                                                                                                                                                      \
        break;                                                                                                                                                                                                 \
    }
            bop(BLT);
            bop(BGT);
            bop(BLE);
            bop(BGE);
#define binsti(type, letter, keyword)                                                                                     \
    case type:                                                                                                            \
        mtd.instructions.push_back(::construct3(::itype::letter##keyword, dest > 0, std::abs(dest), src1.offset, imm16)); \
        break
#define parse_imm16                                                                              \
    auto imm16_var = ::to16(instr.operands[2]);                                                  \
    if (std::holds_alternative<std::string>(imm16_var))                                          \
    {                                                                                            \
        compile_error("Error parsing 16 bit immediate", instr.line_number, instr.column_number); \
        continue;                                                                                \
    }                                                                                            \
    auto imm16 = std::get<std::int16_t>(imm16_var);
#define beopi(keyword)                                                                                                                                                                \
    case ktype::keyword:                                                                                                                                                              \
    {                                                                                                                                                                                 \
        lookup_variable(src1, 1);                                                                                                                                                     \
        lookup_label;                                                                                                                                                                 \
        if (src1.type != 6)                                                                                                                                                           \
        {                                                                                                                                                                             \
            parse_imm16;                                                                                                                                                              \
            mtd.instructions.push_back(::construct3(static_cast<::itype>(static_cast<unsigned>(::itype::I##keyword) + src1.type - 2), dest > 0, std::abs(dest), src1.offset, imm16)); \
            break;                                                                                                                                                                    \
        }                                                                                                                                                                             \
        else                                                                                                                                                                          \
        {                                                                                                                                                                             \
            if (instr.operands[2] != "null")                                                                                                                                          \
            {                                                                                                                                                                         \
                compile_error("Equals immediates may only be null!", instr.line_number, instr.column_number);                                                                         \
                continue;                                                                                                                                                             \
            }                                                                                                                                                                         \
            mtd.instructions.push_back(::construct3(::itype::V##keyword, dest > 0, std::abs(dest), src1.offset, 0));                                                                  \
            break;                                                                                                                                                                    \
        }                                                                                                                                                                             \
    }
            beopi(BEQI);
            beopi(BNEQI);
#define bopi(keyword)                                                                                                                                                                                          \
    case ktype::keyword:                                                                                                                                                                                       \
    {                                                                                                                                                                                                          \
        lookup_variable(src1, 1);                                                                                                                                                                              \
        parse_imm16;                                                                                                                                                                                           \
        lookup_label;                                                                                                                                                                                          \
        switch (src1.type)                                                                                                                                                                                     \
        {                                                                                                                                                                                                      \
            binsti(2, I, keyword);                                                                                                                                                                             \
            binsti(3, L, keyword);                                                                                                                                                                             \
            binsti(4, F, keyword);                                                                                                                                                                             \
            binsti(5, D, keyword);                                                                                                                                                                             \
        default:                                                                                                                                                                                               \
        {                                                                                                                                                                                                      \
            compile_error("Cannot perform comparison operation " << keywords::keyword_to_string[static_cast<unsigned>(instr.itype)] << " on operands of object type", instr.line_number, instr.column_number); \
            continue;                                                                                                                                                                                          \
        }                                                                                                                                                                                                      \
        }                                                                                                                                                                                                      \
        break;                                                                                                                                                                                                 \
    }
            bopi(BLTI);
            bopi(BGTI);
            bopi(BLEI);
            bopi(BGEI);
        case ktype::BU:
        {
            lookup_label;
            mtd.instructions.push_back(::construct32(::itype::BU, dest > 0, std::abs(dest), 0));
            break;
        }
#define require_type(tp, variable, name)                                                                                                               \
    if (variable.type != tp)                                                                                                                           \
    {                                                                                                                                                  \
        compile_error("Wanted type " << tp << " for variable " << name << ", but was type " << variable.type, instr.line_number, instr.column_number); \
        continue;                                                                                                                                      \
    }
        case ktype::ALEN:
        {
            lookup_variable(dest, 0);
            lookup_variable(src1, 1);
            require_type(2, dest, instr.operands[0]);
            require_type(6, src1, instr.operands[1]);
            mtd.instructions.push_back(::construct3(::itype::IVLLD, 0, dest.offset, src1.offset, 0));
            break;
        }
        case ktype::ANEW:
        {
            lookup_variable(dest, 0);
            lookup_variable(src1, 2);
            require_type(6, dest, instr.operands[0]);
            require_type(2, src1, instr.operands[2]);
            if (auto tp = type_map.find(instr.operands[1]); tp != type_map.end())
            {
                mtd.instructions.push_back(::construct3(static_cast<::itype>(static_cast<unsigned>(::itype::CANEW) + tp->second), 0, dest.offset, src1.offset, 0));
            }
            else if (instr.operands[1] == "short")
            {
                mtd.instructions.push_back(::construct3(::itype::SANEW, 0, dest.offset, src1.offset, 0));
            }
            else if (instr.operands[1] == "char")
            {
                mtd.instructions.push_back(::construct3(::itype::CANEW, 0, dest.offset, src1.offset, 0));
            }
            else
            {
                compile_error("Invalid type for array of " << instr.operands[1], instr.line_number, instr.column_number);
                continue;
            }
            break;
        }
        case ktype::CALD:
        {
            lookup_variable(dest, 0);
            lookup_variable(src1, 1);
            lookup_variable(src2, 2);
            require_type(2, dest, instr.operands[0]);
            require_type(6, src1, instr.operands[1]);
            require_type(2, src2, instr.operands[2]);
            mtd.instructions.push_back(::construct3(::itype::CALD, 0, dest.offset, src1.offset, src2.offset));
            break;
        }
        case ktype::SALD:
        {
            lookup_variable(dest, 0);
            lookup_variable(src1, 1);
            lookup_variable(src2, 2);
            require_type(2, dest, instr.operands[0]);
            require_type(6, src1, instr.operands[1]);
            require_type(2, src2, instr.operands[2]);
            mtd.instructions.push_back(::construct3(::itype::SALD, 0, dest.offset, src1.offset, src2.offset));
            break;
        }
        case ktype::ALD:
        {
            lookup_variable(dest, 0);
            lookup_variable(src1, 1);
            lookup_variable(src2, 2);
            require_type(6, src1, instr.operands[1]);
            require_type(2, src2, instr.operands[2]);
            mtd.instructions.push_back(::construct3(static_cast<::itype>(static_cast<unsigned>(::itype::CALD) + dest.type), 0, dest.offset, src1.offset, src2.offset));
            break;
        }
        case ktype::CASR:
        {
            lookup_variable(dest, 0);
            lookup_variable(src1, 1);
            lookup_variable(src2, 2);
            require_type(6, dest, instr.operands[0]);
            require_type(2, src1, instr.operands[1]);
            require_type(2, src2, instr.operands[2]);
            mtd.instructions.push_back(::construct3(::itype::CASR, 0, dest.offset, src1.offset, src2.offset));
            break;
        }
        case ktype::SASR:
        {
            lookup_variable(dest, 0);
            lookup_variable(src1, 1);
            lookup_variable(src2, 2);
            require_type(6, dest, instr.operands[0]);
            require_type(2, src1, instr.operands[1]);
            require_type(2, src2, instr.operands[2]);
            mtd.instructions.push_back(::construct3(::itype::SASR, 0, dest.offset, src1.offset, src2.offset));
            break;
        }
        case ktype::ASR:
        {
            lookup_variable(dest, 0);
            lookup_variable(src1, 1);
            lookup_variable(src2, 2);
            require_type(6, dest, instr.operands[0]);
            require_type(2, src2, instr.operands[2]);
            mtd.instructions.push_back(::construct3(static_cast<::itype>(static_cast<unsigned>(::itype::CASR) + src1.type), 0, dest.offset, src1.offset, src2.offset));
            break;
        }
        case ktype::RET:
        {
            lookup_variable(src1, 0);
            require_type(mtd.return_type, src1, instr.operands[0]);
            mtd.instructions.push_back(::construct3(static_cast<::itype>(static_cast<unsigned>(::itype::IRET) + src1.type - 2), 0, 0, src1.offset, 0));
            break;
        }
        case ktype::NOP:
        {
            mtd.instructions.push_back(::construct40(::itype::NOP, 0, 0));
            break;
        }
        case ktype::IOF:
        {
            lookup_variable(dest, 0);
            lookup_variable(src1, 1);
            require_type(2, dest, instr.operands[0]);
            require_type(6, src1, instr.operands[1]);
            mtd.thunks.push_back({instr.operands[2], static_cast<std::uint16_t>(mtd.instructions.size()), instr.operands[2], location::IMM24, thunk_type::CLASS});
            mtd.instructions.push_back(::construct24(::itype::IOF, dest.offset, src1.offset, 0));
            break;
        }
        case ktype::VNEW:
        {
            lookup_variable(dest, 0);
            require_type(6, dest, instr.operands[0]);
            mtd.thunks.push_back({instr.operands[1], static_cast<std::uint16_t>(mtd.instructions.size()), instr.operands[1], location::IMM24, thunk_type::CLASS});
            mtd.instructions.push_back(::construct24(::itype::VNEW, dest.offset, 0, 0));
            break;
        }
        case ktype::CVLLD:
        {
            lookup_variable(dest, 0);
            lookup_variable(src1, 1);
            require_type(2, dest, instr.operands[0]);
            require_type(6, src1, instr.operands[1]);
            mtd.thunks.push_back({instr.operands[2].substr(instr.operands[2].find_last_of('.') + 1), static_cast<std::uint16_t>(mtd.instructions.size()), instr.operands[2].substr(0, instr.operands[2].find_last_of('.')), location::IMM24, thunk_type::IVAR});
            mtd.instructions.push_back(::construct24(::itype::CVLLD, dest.offset, src1.offset, 0));
            break;
        }
        case ktype::SVLLD:
        {
            lookup_variable(dest, 0);
            lookup_variable(src1, 1);
            require_type(2, dest, instr.operands[0]);
            require_type(6, src1, instr.operands[1]);
            mtd.thunks.push_back({instr.operands[2].substr(instr.operands[2].find_last_of('.') + 1), static_cast<std::uint16_t>(mtd.instructions.size()), instr.operands[2].substr(0, instr.operands[2].find_last_of('.')), location::IMM24, thunk_type::IVAR});
            mtd.instructions.push_back(::construct24(::itype::SVLLD, dest.offset, src1.offset, 0));
            break;
        }
        case ktype::VLLD:
        {
            lookup_variable(dest, 0);
            lookup_variable(src1, 1);
            require_type(6, src1, instr.operands[1]);
            mtd.thunks.push_back({instr.operands[2].substr(instr.operands[2].find_last_of('.') + 1), static_cast<std::uint16_t>(mtd.instructions.size()), instr.operands[2].substr(0, instr.operands[2].find_last_of('.')), location::IMM24, thunk_type::IVAR});
            mtd.instructions.push_back(::construct24(static_cast<::itype>(static_cast<unsigned>(::itype::CVLLD) + dest.type), dest.offset, src1.offset, 0));
            break;
        }
        case ktype::CVLSR:
        {
            lookup_variable(dest, 0);
            lookup_variable(src1, 1);
            require_type(6, dest, instr.operands[0]);
            require_type(2, src1, instr.operands[1]);
            mtd.thunks.push_back({instr.operands[2].substr(instr.operands[2].find_last_of('.') + 1), static_cast<std::uint16_t>(mtd.instructions.size()), instr.operands[2].substr(0, instr.operands[2].find_last_of('.')), location::IMM24, thunk_type::IVAR});
            mtd.instructions.push_back(::construct24(::itype::CVLSR, dest.offset, src1.offset, 0));
            break;
        }
        case ktype::SVLSR:
        {
            lookup_variable(dest, 0);
            lookup_variable(src1, 1);
            require_type(6, dest, instr.operands[0]);
            require_type(2, src1, instr.operands[1]);
            mtd.thunks.push_back({instr.operands[2].substr(instr.operands[2].find_last_of('.') + 1), static_cast<std::uint16_t>(mtd.instructions.size()), instr.operands[2].substr(0, instr.operands[2].find_last_of('.')), location::IMM24, thunk_type::IVAR});
            mtd.instructions.push_back(::construct24(::itype::SVLSR, dest.offset, src1.offset, 0));
            break;
        }
        case ktype::VLSR:
        {
            lookup_variable(dest, 0);
            lookup_variable(src1, 1);
            require_type(6, dest, instr.operands[0]);
            mtd.thunks.push_back({instr.operands[2].substr(instr.operands[2].find_last_of('.') + 1), static_cast<std::uint16_t>(mtd.instructions.size()), instr.operands[2].substr(0, instr.operands[2].find_last_of('.')), location::IMM24, thunk_type::IVAR});
            mtd.instructions.push_back(::construct24(static_cast<::itype>(static_cast<unsigned>(::itype::CVLSR) + src1.type), dest.offset, src1.offset, 0));
            break;
        }
        case ktype::CSTLD:
        {
            lookup_variable(dest, 0);
            require_type(2, dest, instr.operands[0]);
            mtd.thunks.push_back({instr.operands[1].substr(instr.operands[1].find_last_of('.') + 1), static_cast<std::uint16_t>(mtd.instructions.size()), instr.operands[1].substr(0, instr.operands[1].find_last_of('.')), location::IMM32, thunk_type::SVAR});
            mtd.instructions.push_back(::construct32(::itype::CSTLD, 0, dest.offset, 0));
            break;
        }
        case ktype::SSTLD:
        {
            lookup_variable(dest, 0);
            require_type(2, dest, instr.operands[0]);
            mtd.thunks.push_back({instr.operands[1].substr(instr.operands[1].find_last_of('.') + 1), static_cast<std::uint16_t>(mtd.instructions.size()), instr.operands[1].substr(0, instr.operands[1].find_last_of('.')), location::IMM32, thunk_type::SVAR});
            mtd.instructions.push_back(::construct32(::itype::SSTLD, 0, dest.offset, 0));
            break;
        }
        case ktype::STLD:
        {
            lookup_variable(dest, 0);
            mtd.thunks.push_back({instr.operands[1].substr(instr.operands[1].find_last_of('.') + 1), static_cast<std::uint16_t>(mtd.instructions.size()), instr.operands[1].substr(0, instr.operands[1].find_last_of('.')), location::IMM32, thunk_type::SVAR});
            mtd.instructions.push_back(::construct32(static_cast<::itype>(static_cast<unsigned>(::itype::CSTLD) + dest.type), 0, dest.offset, 0));
            break;
        }
        case ktype::CSTSR:
        {
            lookup_variable(dest, 0);
            require_type(2, dest, instr.operands[0]);
            mtd.thunks.push_back({instr.operands[1].substr(instr.operands[1].find_last_of('.') + 1), static_cast<std::uint16_t>(mtd.instructions.size()), instr.operands[1].substr(0, instr.operands[1].find_last_of('.')), location::IMM32, thunk_type::SVAR});
            mtd.instructions.push_back(::construct32(::itype::CSTSR, 0, dest.offset, 0));
            break;
        }
        case ktype::SSTSR:
        {
            lookup_variable(dest, 0);
            require_type(2, dest, instr.operands[0]);
            mtd.thunks.push_back({instr.operands[1].substr(instr.operands[1].find_last_of('.') + 1), static_cast<std::uint16_t>(mtd.instructions.size()), instr.operands[1].substr(0, instr.operands[1].find_last_of('.')), location::IMM32, thunk_type::SVAR});
            mtd.instructions.push_back(::construct32(::itype::SSTSR, 0, dest.offset, 0));
            break;
        }
        case ktype::STSR:
        {
            lookup_variable(dest, 0);
            mtd.thunks.push_back({instr.operands[1].substr(instr.operands[1].find_last_of('.') + 1), static_cast<std::uint16_t>(mtd.instructions.size()), instr.operands[1].substr(0, instr.operands[1].find_last_of('.')), location::IMM32, thunk_type::SVAR});
            mtd.instructions.push_back(::construct32(static_cast<::itype>(static_cast<unsigned>(::itype::CSTSR) + dest.type), 0, dest.offset, 0));
            break;
        }
#define load_args                                                                                                     \
    std::uint64_t arg_builder = 0;                                                                                    \
    for (unsigned i = 0; i < instr.operands.size() - 2; i++)                                                          \
    {                                                                                                                 \
        lookup_variable(arg, i + 2);                                                                                  \
        arg_builder |= static_cast<std::uint64_t>(arg.offset) << (i % 4 * CHAR_BIT * sizeof(std::uint16_t));          \
        if (i % (sizeof(std::uint64_t) / sizeof(std::uint16_t)) == sizeof(std::uint64_t) / sizeof(std::uint16_t) - 1) \
        {                                                                                                             \
            mtd.instructions.push_back(arg_builder);                                                                  \
            arg_builder = 0;                                                                                          \
        }                                                                                                             \
    }                                                                                                                 \
    if (instr.operands.size() % (sizeof(std::uint64_t) / sizeof(std::uint16_t)) != 2)                                 \
    {                                                                                                                 \
        mtd.instructions.push_back(arg_builder);                                                                      \
    }
        case ktype::SINV:
        {
            lookup_variable(dest, 0);
            mtd.thunks.push_back({instr.operands[1].substr(instr.operands[1].find_last_of('.') + 1), static_cast<std::uint16_t>(mtd.instructions.size()), instr.operands[1].substr(0, instr.operands[1].find_last_of('.')), location::IMM32, thunk_type::METHOD});
            mtd.instructions.push_back(::construct32(::itype::SINV, 0, dest.offset, 0));
            load_args;
            break;
        }
        case ktype::IINV:
        {
            lookup_variable(dest, 0);
            lookup_variable(src1, 1);
            mtd.thunks.push_back({instr.operands[2].substr(instr.operands[2].find_last_of('.') + 1), static_cast<std::uint16_t>(mtd.instructions.size()), instr.operands[2].substr(0, instr.operands[2].find_last_of('.')), location::IMM24, thunk_type::METHOD});
            mtd.instructions.push_back(::construct24(::itype::IINV, dest.offset, src1.offset, 0));
            load_args;
            break;
        }
        case ktype::VINV:
        {
            lookup_variable(dest, 0);
            lookup_variable(src1, 1);
            mtd.thunks.push_back({instr.operands[2].substr(instr.operands[2].find_last_of('.') + 1), static_cast<std::uint16_t>(mtd.instructions.size()), instr.operands[2].substr(0, instr.operands[2].find_last_of('.')), location::IMM24, thunk_type::METHOD});
            mtd.instructions.push_back(::construct24(::itype::VINV, dest.offset, src1.offset, 0));
            load_args;
            break;
        }
        case ktype::LBL:
        case ktype::DEF:
            break;
        case ktype::BCMP:
        case ktype::BADR:
        case ktype::EXC:
        {
            compile_error("Instruction type " << keywords::keyword_to_string[static_cast<unsigned>(instr.itype)] << " is not supported yet!", instr.line_number, instr.column_number);
            continue;
        }
        case ktype::IMP:
        case ktype::IVAR:
        case ktype::SVAR:
        case ktype::PROC:
        case ktype::EPROC:
        case ktype::EXT:
        case ktype::IMPL:
        case ktype::CLZ:
        {
            compile_error("Instruction type " << keywords::keyword_to_string[static_cast<unsigned>(instr.itype)] << " is not allowed within the body of a method!", instr.line_number, instr.column_number);
            continue;
        }
        }
    }
    mtd.size = sizeof(char *);
    mtd.size += sizeof(std::uint16_t) * 4;
    mtd.size += ::round_off(mtd.arg_types.size(), CHAR_BIT * sizeof(std::uint64_t) / 4) / (sizeof(std::uint64_t) * CHAR_BIT / 4) * sizeof(std::uint64_t);
    mtd.size += mtd.instructions.size() * sizeof(std::uint64_t);
    mtd.size += sizeof(char *);
    mtd.size += ::round_off(mtd.handle_map.size() + 1, sizeof(std::uint64_t) / sizeof(std::uint16_t)) / (sizeof(std::uint64_t) / sizeof(std::uint16_t)) * sizeof(std::uint64_t);
    return mtd;
}