#include "compiler.h"

#include <sstream>

#include "../instructions/keywords.h"
#include "../utils/hashing.h"

using namespace oops_bcode_compiler::compiler;

namespace
{
    struct var
    {
        std::uint16_t offset;
        std::uint8_t type;
    };
    enum class itype : unsigned char
    {
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
        //TODO
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
        CANEW,
        SANEW,
        IANEW,
        LANEW,
        FANEW,
        DANEW,
        VANEW,
        IOF,
        VINV,
        SINV,
        IINV,
        IRET,
        LRET,
        FRET,
        DRET,
        VRET,
        //TODO
        EXC
    };

    std::uint64_t construct(itype type, std::uint8_t flags, std::uint16_t dest, std::uint16_t src1, std::uint16_t src2)
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
    std::uint64_t construct(itype type, std::uint16_t dest, std::uint16_t src1, std::uint32_t imm24)
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
    std::uint64_t construct(itype type, std::uint8_t flags, std::uint16_t dest, std::uint32_t imm32)
    {
        std::uint64_t out = 0;
        out <<= CHAR_BIT * sizeof(type);
        out |= static_cast<std::uint8_t>(type);
        out <<= CHAR_BIT * sizeof(sizeof(flags));
        out |= static_cast<std::uint8_t>(flags);
        out <<= CHAR_BIT * sizeof(imm32);
        out |= imm32;
        out <<= CHAR_BIT * sizeof(dest);
        out |= dest;
        return out;
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
        catch (std::invalid_argument)
        {
            pfail;
        }
        catch (std::out_of_range)
        {
            return "'" + str + "' is too large to be an integer";
        }
        return parsed;
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
        catch (std::invalid_argument)
        {
            pfail;
        }
        catch (std::out_of_range)
        {
            return "'" + str + "' is too large to be a long";
        }
        return parsed;
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
        catch (std::invalid_argument)
        {
            return "'" + str + "' could not be parsed as a float";
        }
        catch (std::out_of_range)
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
        catch (std::invalid_argument)
        {
            return "'" + str + "' could not be parsed as a double";
        }
        catch (std::out_of_range)
        {
            return "'" + str + "' is too large to be a double";
        }
    }

    std::variant<std::int32_t, std::string> to24(const std::string &str)
    {
        auto parsed = parse_int(str);
        if (std::holds_alternative<std::int32_t>(parsed))
        {
            if (std::get<std::int32_t>(parsed) & ~static_cast<std::int32_t>(0) << 24)
            {
                return "'" + str + "' is too large to be a 24-bit integer";
            }
        }
        return parsed;
    }
} // namespace

std::variant<method, std::string> oops_bcode_compiler::compiler::compile(const oops_bcode_compiler::parsing::cls::procedure &proc, std::stringstream &error_builder)
{
    static const std::unordered_map<std::string, std::size_t> sizer = {{"int", sizeof(std::int32_t) / sizeof(std::uint32_t)}, {"long", sizeof(std::int64_t) / sizeof(std::uint32_t)}, {"float", sizeof(float) / sizeof(std::uint32_t)}, {"double", sizeof(double) / sizeof(std::uint32_t)}};
    static const std::unordered_map<std::string, std::size_t> typer = {{"int", 2}, {"long", 3}, {"float", 4}, {"double", 5}};
    method mtd;
    mtd.stack_size = 0;
    mtd.size = 0;
    std::unordered_map<std::string, ::var> local_variables;
    for (auto arg : proc.parameters)
    {
        if (auto type = typer.find(arg.host_name); type != typer.end())
        {
            mtd.arg_types.push_back(type->second);
            local_variables[arg.name] = {mtd.stack_size, type->second};
        }
        else
        {
            mtd.handle_map.push_back(mtd.stack_size);
            mtd.arg_types.push_back(6);
            local_variables[arg.name] = {mtd.stack_size, 6};
        }
        mtd.stack_size += sizeof(std::uint64_t) / sizeof(std::uint32_t);
    }
    auto rtype = typer.find(proc.return_type_name);
    mtd.return_type = rtype != typer.end() ? rtype->second : 6;
    for (std::size_t i = 0; i < proc.instructions.size(); i++)
    {
        const auto &instr = proc.instructions[i];
#define compiling_error(error)                                                                                                                                                                                                                                      \
    error_builder << "Error while compiling procedure " << proc.name << ": " << error << " for instruction " << i << " (" << keywords::keyword_to_string[static_cast<unsigned>(instr.itype)] << " " << instr.dest << " " << instr.src1 << " " << instr.src2 << ")"; \
    return error_builder.str()
        std::uint64_t instruction;
        switch (instr.itype)
        {
#define ctype(type, prefix, ktype)                                              \
    case type:                                                                  \
    {                                                                           \
        instruction = ::construct(::itype::prefix##ktype, 0, dest, src1, src2); \
        break;                                                                  \
    }
#define c24type(type, prefix, ktype)                                          \
    case type:                                                                \
    {                                                                         \
        instruction = ::construct(::itype::prefix##ktype, dest, src1, imm24); \
        break;                                                                \
    }
#pragma region

#define basic_op(ktype, tswitch)                                                                                 \
    case keywords::keyword::ktype:                                                                               \
    {                                                                                                            \
        std::uint16_t src2, src1, dest;                                                                          \
        unsigned type;                                                                                           \
        if (auto it = local_variables.find(instr.src2); it != local_variables.end())                             \
        {                                                                                                        \
            src2 = it->second.offset;                                                                            \
            type = it->second.type;                                                                              \
        }                                                                                                        \
        else                                                                                                     \
        {                                                                                                        \
            compiling_error("Undefined local variable " << instr.src2);                                          \
        }                                                                                                        \
        if (auto it = local_variables.find(instr.src1); it != local_variables.end())                             \
        {                                                                                                        \
            src1 = it->second.offset;                                                                            \
            if (type != it->second.type)                                                                         \
            {                                                                                                    \
                compiling_error("Mismatched types for src1 and src2 of " << it->second.type << " and " << type); \
            }                                                                                                    \
        }                                                                                                        \
        else                                                                                                     \
        {                                                                                                        \
            compiling_error("Undefined local variable " << instr.src1);                                          \
        }                                                                                                        \
        if (auto it = local_variables.find(instr.dest); it != local_variables.end())                             \
        {                                                                                                        \
            dest = it->second.offset;                                                                            \
            if (type != it->second.type)                                                                         \
            {                                                                                                    \
                compiling_error("Mismatched types for dest and src2 of " << it->second.type << " and " << type); \
            }                                                                                                    \
        }                                                                                                        \
        else                                                                                                     \
        {                                                                                                        \
            compiling_error("Undefined local variable " << instr.dest);                                          \
        }                                                                                                        \
        switch (type)                                                                                            \
        {                                                                                                        \
            tswitch;                                                                                             \
        default:                                                                                                 \
            compiling_error("Unsupported type " << type);                                                        \
        }                                                                                                        \
        break;                                                                                                   \
    }
#pragma endregion
#define int_op(ktype) basic_op(ktype, ctype(2, I, ktype); ctype(3, L, ktype))
#define prim_op(ktype) basic_op(ktype, ctype(2, I, ktype); ctype(3, L, ktype); ctype(4, F, ktype); ctype(5, D, ktype))
            prim_op(ADD);
            prim_op(SUB);
            prim_op(MUL);
            prim_op(DIV);
            int_op(DIVU);
            int_op(AND);
            int_op(OR);
            int_op(XOR);
            int_op(SLL);
            int_op(SRL);
            int_op(SRA);
#pragma region

#define basic_imm_op(ktype, tswitch)                                                                             \
    case keywords::keyword::ktype:                                                                               \
    {                                                                                                            \
        std::uint32_t imm24;                                                                                     \
        std::uint16_t src1, dest;                                                                                \
        unsigned type;                                                                                           \
        if (auto it = local_variables.find(instr.src1); it != local_variables.end())                             \
        {                                                                                                        \
            src1 = it->second.offset;                                                                            \
            type = it->second.type;                                                                              \
        }                                                                                                        \
        else                                                                                                     \
        {                                                                                                        \
            compiling_error("Undefined local variable " << instr.src1);                                          \
        }                                                                                                        \
        if (auto it = local_variables.find(instr.dest); it != local_variables.end())                             \
        {                                                                                                        \
            dest = it->second.offset;                                                                            \
            if (type != it->second.type)                                                                         \
            {                                                                                                    \
                compiling_error("Mismatched types for dest and src1 of " << it->second.type << " and " << type); \
            }                                                                                                    \
        }                                                                                                        \
        else                                                                                                     \
        {                                                                                                        \
            compiling_error("Undefined local variable " << instr.dest);                                          \
        }                                                                                                        \
        auto parsed = to24(instr.src2);                                                                          \
        if (std::holds_alternative<std::int32_t>(parsed))                                                        \
        {                                                                                                        \
            imm24 = std::get<std::int32_t>(parsed);                                                              \
        }                                                                                                        \
        else                                                                                                     \
        {                                                                                                        \
            compiling_error(std::get<std::string>(parsed));                                                      \
        }                                                                                                        \
        switch (type)                                                                                            \
        {                                                                                                        \
            tswitch;                                                                                             \
        default:                                                                                                 \
            compiling_error("Unsupported type " << type);                                                        \
        }                                                                                                        \
        break;                                                                                                   \
    }
#pragma endregion
#define int_imm_op(ktype) basic_imm_op(ktype, c24type(2, I, ktype); c24type(3, L, ktype))
#define prim_imm_op(ktype) basic_imm_op(ktype, c24type(2, I, ktype); c24type(3, L, ktype); c24type(4, F, ktype); c24type(5, D, ktype))
            prim_imm_op(ADDI);
            prim_imm_op(SUBI);
            prim_imm_op(MULI);
            prim_imm_op(DIVI);
            int_imm_op(DIVUI);
            int_imm_op(ANDI);
            int_imm_op(ORI);
            int_imm_op(XORI);
            int_imm_op(SLLI);
            int_imm_op(SRLI);
            int_imm_op(SRAI);
        }
        mtd.instructions.push_back(instruction);
    }
    return mtd;
}