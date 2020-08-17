#include "compiler.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include "../instructions/keywords.h"
#include "../utils/hashing.h"
#include "../utils/puns.h"

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
            if (static_cast<std::uint32_t>(std::get<std::int32_t>(parsed)) >> 24)
            {
                return "'" + str + "' is too large to be a 24-bit integer";
            }
        }
        return parsed;
    }
    std::variant<std::int16_t, std::string> to16(const std::string &str)
    {
        auto parsed = parse_int(str);
        if (std::holds_alternative<std::int32_t>(parsed))
        {
            if (static_cast<std::uint32_t>(std::get<std::int32_t>(parsed)) >> 16)
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
} // namespace

std::variant<method, std::string> oops_bcode_compiler::compiler::compile(const oops_bcode_compiler::parsing::cls::procedure &proc, std::stringstream &error_builder)
{
    static const std::unordered_map<std::string, std::uint8_t> sizer = {{"int", sizeof(std::int32_t) / sizeof(std::uint32_t)}, {"long", sizeof(std::int64_t) / sizeof(std::uint32_t)}, {"float", sizeof(float) / sizeof(std::uint32_t)}, {"double", sizeof(double) / sizeof(std::uint32_t)}};
    static const std::unordered_map<std::string, std::uint8_t> typer = {{"int", 2}, {"long", 3}, {"float", 4}, {"double", 5}};
    method mtd;
    mtd.stack_size = 0;
    mtd.size = 0;
    std::unordered_map<std::string, ::var> local_variables;
    std::unordered_map<std::string, std::uint16_t> labels;
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
    mtd.method_type = proc.is_static ? 4 : 5;
    mtd.name = proc.name;
    std::uint16_t iidx = 0;
    unsigned arg_counter = 0;
    std::vector<std::tuple<std::uint8_t, std::uint8_t, std::string>> defines;
    for (std::size_t i = 0; i < proc.instructions.size(); i++)
    {
        auto &instr = proc.instructions[i];
#define compiling_error(error)                                                                                                                                                                                                                                      \
    error_builder << "Error while compiling procedure " << proc.name << ": " << error << " for instruction " << i << " (" << keywords::keyword_to_string[static_cast<unsigned>(instr.itype)] << " " << instr.dest << " " << instr.src1 << " " << instr.src2 << ")"; \
    return error_builder.str()
#define lookup_variable(var)                                 \
    auto var = local_variables.find(instr.var);              \
    if (var == local_variables.end())                        \
    {                                                        \
        compiling_error("Undefined variable " << instr.var); \
    }
#define require_type(var, idx)                                                                          \
    if (var->second.type != idx)                                                                        \
    {                                                                                                   \
        compiling_error("Wanted " #var " to have type " #idx ", but was instead " << var->second.type); \
    }
        if (instr.itype == keywords::keyword::PASS)
        {
            arg_counter++;
            continue;
        }
        iidx += (arg_counter + 3) / 4;
        switch (instr.itype)
        {
        case keywords::keyword::LBL:
            labels[instr.src1] = iidx;
            continue;
        case keywords::keyword::DEF:
        {
            auto size = sizer.find(instr.dest);
            if (size != sizer.end())
            {
                defines.push_back({size->second, typer.find(instr.dest)->second, instr.src1});
            }
            else
            {
                defines.push_back({sizeof(char *) / sizeof(std::int32_t), 6, instr.src1});
            }
            local_variables[instr.dest] = {0, std::get<1>(defines.back())};
            continue;
        }
        case keywords::keyword::LI:
        {
            lookup_variable(dest);
            switch (dest->second.type)
            {
            case 2:
            case 4:
            case 6:
            {
                iidx++;
                break;
            }
            case 3:
            {
                auto parsed = ::parse_long(instr.src1);
                if (std::holds_alternative<std::int64_t>(parsed))
                {
                    iidx += 1 + ((std::get<std::int64_t>(parsed) << CHAR_BIT * (sizeof(std::uint16_t) * 2 + sizeof(std::uint8_t))) == 0);
                }
                else
                {
                    compiling_error(std::get<std::string>(parsed));
                }
                break;
            }
            case 5:
            {
                auto parsed = ::parse_double(instr.src1);
                if (std::holds_alternative<std::string>(parsed))
                {
                    compiling_error(std::get<std::string>(parsed));
                }
                iidx += 1 + ((utils::pun_reinterpret<std::uint64_t>(std::get<double>(parsed)) << CHAR_BIT * (sizeof(std::uint16_t) * 2 + sizeof(std::uint8_t))) == 0);
                break;
            }
            }
            continue;
        }
        case keywords::keyword::IMP:
        case keywords::keyword::IVAR:
        case keywords::keyword::SVAR:
        case keywords::keyword::ARG:
        case keywords::keyword::PROC:
        case keywords::keyword::SPROC:
        case keywords::keyword::EPROC:
        case keywords::keyword::EXT:
        case keywords::keyword::IMPL:
        case keywords::keyword::CLZ:
            compiling_error("Invalid keyword in procedure");
        default:
            break;
        }
        iidx++;
    }
    std::sort(defines.begin(), defines.end());
    std::for_each(defines.rbegin(), defines.rend(), [&mtd, &local_variables](decltype(defines)::value_type tup) {
        local_variables[std::get<2>(tup)] = {mtd.stack_size, std::get<1>(tup)};
        if (std::get<1>(tup) == 6)
        {
            mtd.handle_map.push_back(mtd.stack_size);
        }
        mtd.stack_size += std::get<0>(tup);
    });
    std::uint64_t arg_builder = 0;
    for (std::size_t i = 0; i < proc.instructions.size(); i++)
    {
        const auto &instr = proc.instructions[i];
        if (instr.itype != keywords::keyword::PASS)
        {
            if (arg_counter % 4 != 0)
            {
                mtd.instructions.push_back(arg_builder >> CHAR_BIT * sizeof(std::uint16_t) * (arg_counter % 4));
                arg_builder = 0;
                arg_counter = 0;
            }
        }
        else
        {
            arg_builder >>= CHAR_BIT * sizeof(std::uint16_t);
            lookup_variable(dest);
            arg_builder |= static_cast<std::uint64_t>(dest->second.offset) << CHAR_BIT * (sizeof(std::uint64_t) - sizeof(std::uint16_t));
            if (arg_counter % 4 == 3)
            {
                mtd.instructions.push_back(arg_builder);
                arg_builder = 0;
            }
            arg_counter++;
            continue;
        }
        std::uint64_t instruction;
        switch (instr.itype)
        {
#define typecheck(var1, var2)                                                                                                          \
    if (var1->second.type != var2->second.type)                                                                                        \
    {                                                                                                                                  \
        compiling_error("Types of " #var1 " and " #var2 " do not match (" << var1->second.type << " vs " << var2->second.type << ")"); \
    }
#define ctype(type, prefix, ktype)                                                                                            \
    case type:                                                                                                                \
    {                                                                                                                         \
        instruction = ::construct3(::itype::prefix##ktype, 0, dest->second.offset, src1->second.offset, src2->second.offset); \
        break;                                                                                                                \
    }
#define c24type(type, prefix, ktype)                                                                         \
    case type:                                                                                               \
    {                                                                                                        \
        instruction = ::construct24(::itype::prefix##ktype, dest->second.offset, src1->second.offset, src2); \
        break;                                                                                               \
    }
#define basic_op(ktype, tswitch)                                       \
    case keywords::keyword::ktype:                                     \
    {                                                                  \
        lookup_variable(src1);                                         \
        lookup_variable(src2);                                         \
        lookup_variable(dest);                                         \
        typecheck(src1, src2);                                         \
        typecheck(src1, dest);                                         \
        switch (dest->second.type)                                     \
        {                                                              \
            tswitch;                                                   \
        default:                                                       \
            compiling_error("Unsupported type " << dest->second.type); \
        }                                                              \
        break;                                                         \
    }
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
#define parse(var, pfunc, type)                         \
    auto parsed = pfunc(instr.var);                     \
    if (std::holds_alternative<std::string>(parsed))    \
    {                                                   \
        compiling_error(std::get<std::string>(parsed)); \
    }                                                   \
    auto var = std::get<type>(parsed);
#define basic_imm_op(ktype, tswitch)                                   \
    case keywords::keyword::ktype:                                     \
    {                                                                  \
        lookup_variable(src1);                                         \
        lookup_variable(dest);                                         \
        typecheck(src1, dest);                                         \
        parse(src2, to24, std::int32_t);                               \
        switch (dest->second.type)                                     \
        {                                                              \
            tswitch;                                                   \
        default:                                                       \
            compiling_error("Unsupported type " << dest->second.type); \
        }                                                              \
        break;                                                         \
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
        case keywords::keyword::ANEW:
        {
            lookup_variable(src2);
            lookup_variable(dest);
            require_type(src2, 2);
            require_type(dest, 6);
            if (auto type = typer.find(instr.src1); type != typer.end())
            {
                switch (type->second)
                {
                case 2:
                    instruction = ::construct3(::itype::IANEW, 0, dest->second.offset, src2->second.offset, 0);
                    break;
                case 3:
                    instruction = ::construct3(::itype::LANEW, 0, dest->second.offset, src2->second.offset, 0);
                    break;
                case 4:
                    instruction = ::construct3(::itype::FANEW, 0, dest->second.offset, src2->second.offset, 0);
                    break;
                case 5:
                    instruction = ::construct3(::itype::DANEW, 0, dest->second.offset, src2->second.offset, 0);
                    break;
                }
            }
            else if (instr.src1 == "char")
            {
                instruction = ::construct3(::itype::CANEW, 0, dest->second.offset, src2->second.offset, 0);
            }
            else if (instr.src1 == "short")
            {
                instruction = ::construct3(::itype::SANEW, 0, dest->second.offset, src2->second.offset, 0);
            }
            else
            {
                instruction = ::construct3(::itype::VANEW, 0, dest->second.offset, src2->second.offset, 0);
            }
            break;
        }
#define cbr(idx, type, ktype)                                                                                                                                                                                                                                                                                       \
    case idx:                                                                                                                                                                                                                                                                                                       \
        instruction = ::construct3(::itype::type##ktype, to_instr->second < static_cast<std::uint16_t>(mtd.instructions.size()), static_cast<std::int16_t>(std::abs(static_cast<std::int32_t>(static_cast<std::uint16_t>(mtd.instructions.size())) - to_instr->second)), src1->second.offset, src2->second.offset); \
        break
#define lookup_label                                               \
    auto to_instr = labels.find(instr.dest);                       \
    if (to_instr == labels.end())                                  \
    {                                                              \
        compiling_error("Undefined label '" << instr.dest << "'"); \
    }
#define branch(ktype, tswitch)     \
    case keywords::keyword::ktype: \
    {                              \
        lookup_label;              \
        lookup_variable(src1);     \
        lookup_variable(src2);     \
        typecheck(src1, src2);     \
        switch (src1->second.type) \
        {                          \
            tswitch(ktype);        \
        }                          \
        break;                     \
    }
#define cmp(ktype)    \
    cbr(2, I, ktype); \
    cbr(3, L, ktype); \
    cbr(4, F, ktype); \
    cbr(5, D, ktype)
#define eq(ktype) \
    cmp(ktype);   \
    cbr(6, V, ktype)
            branch(BEQ, eq);
            branch(BNEQ, eq);
            branch(BLE, cmp);
            branch(BLT, cmp);
            branch(BGE, cmp);
            branch(BGT, cmp);
#undef cbr

#define branch_imm(ktype, tswitch)       \
    case keywords::keyword::ktype:       \
    {                                    \
        lookup_label;                    \
        lookup_variable(src1);           \
        parse(src2, to16, std::int16_t); \
        switch (src1->second.type)       \
        {                                \
            tswitch(ktype);              \
        }                                \
        break;                           \
    }
#define cbr(idx, type, ktype)                                                                                                                                                                                                                                                                        \
    case idx:                                                                                                                                                                                                                                                                                        \
        instruction = ::construct3(::itype::type##ktype, to_instr->second < static_cast<std::uint16_t>(mtd.instructions.size()), static_cast<std::int16_t>(std::abs(static_cast<std::int32_t>(static_cast<std::uint16_t>(mtd.instructions.size())) - to_instr->second)), src1->second.offset, src2); \
        break
            branch_imm(BEQI, eq);
            branch_imm(BNEQI, eq);
            branch_imm(BLEI, cmp);
            branch_imm(BLTI, cmp);
            branch_imm(BGEI, cmp);
            branch_imm(BGTI, cmp);
        case keywords::keyword::BU:
        {
            lookup_label;
            instruction = ::construct3(::itype::BU, to_instr->second < i, static_cast<std::int16_t>(std::abs(static_cast<std::int32_t>(static_cast<std::uint16_t>(mtd.instructions.size())) - to_instr->second)), 0, 0);
            break;
        }
        case keywords::keyword::NEG:
        {
            lookup_variable(dest);
            lookup_variable(src1);
            typecheck(src1, dest);
            switch (src1->second.type)
            {
            case 2:
                instruction = ::construct3(::itype::INEG, 0, dest->second.offset, src1->second.offset, 0);
                break;
            case 3:
                instruction = ::construct3(::itype::LNEG, 0, dest->second.offset, src1->second.offset, 0);
                break;
            case 4:
                instruction = ::construct3(::itype::FNEG, 0, dest->second.offset, src1->second.offset, 0);
                break;
            case 5:
                instruction = ::construct3(::itype::DNEG, 0, dest->second.offset, src1->second.offset, 0);
                break;
            default:
                compiling_error("Unsupported type " << src1->second.type);
            }
            break;
        }
        case keywords::keyword::LI:
        {
            lookup_variable(dest);
            switch (dest->second.type)
            {
            case 2:
            {
                parse(src1, parse_int, std::int32_t);
                instruction = ::construct32(::itype::LDI, 0, dest->second.offset, src1);
                break;
            }
            case 3:
            {
                parse(src1, parse_long, std::int64_t);
                instruction = ::construct40(::itype::LUI, dest->second.offset, src1 >> 24);
                if ((src1 << (sizeof(std::uint64_t) * CHAR_BIT - 24)) != 0)
                {
                    mtd.instructions.push_back(instruction);
                    instruction = ::construct24(::itype::LADDI, dest->second.offset, dest->second.offset, src1 << (sizeof(std::uint64_t) * CHAR_BIT - 24) >> (sizeof(std::uint64_t) * CHAR_BIT - 24));
                }
                break;
            }
            case 4:
            {
                parse(src1, parse_float, float);
                instruction = ::construct32(::itype::LDI, 0, dest->second.offset, utils::pun_reinterpret<std::int32_t>(src1));
                break;
            }
            case 5:
            {
                parse(src1, parse_double, double);
                std::uint64_t big = utils::pun_reinterpret<std::uint64_t>(src1);
                instruction = ::construct40(::itype::LUI, dest->second.offset, big >> 24);
                if (big << (sizeof(std::uint64_t) * CHAR_BIT - 24))
                {
                    mtd.instructions.push_back(instruction);
                    instruction = ::construct24(::itype::LADDI, dest->second.offset, dest->second.offset, big << (sizeof(std::uint64_t) * CHAR_BIT - 24) >> (sizeof(std::uint64_t) * CHAR_BIT - 24));
                }
                break;
            }
            case 6:
            {
                if (instr.src1 != "null")
                {
                    compiling_error("Cannot load non-null value " << instr.src1 << " into reference variable " << instr.dest);
                }
                instruction = ::construct40(::itype::LNL, dest->second.offset, 0);
            }
            }
            break;
        }
        case keywords::keyword::CST:
        {
            lookup_variable(dest);
            lookup_variable(src1);
            if (src1->second.type == dest->second.type)
            {
                compiling_error("Casting between two variables of same type " << src1->second.type << " is not allowed");
            }
            if (src1->second.type == 6)
            {
                compiling_error(instr.src1 << " is a reference variable and cannot be cast");
            }
            if (dest->second.type == 6)
            {
                compiling_error(instr.dest << " is a reference variable and cannot be cast");
            }
            ::itype type;
            switch (::cast_types(src1->second.type, dest->second.type))
            {
            case ::cast_types(2, 3):
                type = ::itype::ICSTL;
                break;
            case ::cast_types(2, 4):
                type = ::itype::ICSTF;
                break;
            case ::cast_types(2, 5):
                type = ::itype::ICSTD;
                break;
            case ::cast_types(3, 2):
                type = ::itype::LCSTI;
                break;
            case ::cast_types(3, 4):
                type = ::itype::LCSTF;
                break;
            case ::cast_types(3, 5):
                type = ::itype::LCSTD;
                break;
            case ::cast_types(4, 2):
                type = ::itype::FCSTI;
                break;
            case ::cast_types(4, 3):
                type = ::itype::FCSTL;
                break;
            case ::cast_types(4, 5):
                type = ::itype::FCSTD;
                break;
            case ::cast_types(5, 2):
                type = ::itype::DCSTI;
                break;
            case ::cast_types(5, 3):
                type = ::itype::DCSTL;
                break;
            case ::cast_types(5, 4):
                type = ::itype::DCSTF;
                break;
            default:
                compiling_error("Invalid combination of types for src1 and dest (" << src1->second.type << " and " << dest->second.type << ")");
            }
            instruction = ::construct3(type, 0, dest->second.offset, src1->second.offset, 0);
            break;
        }
        case keywords::keyword::VNEW:
        {
            lookup_variable(dest);
            mtd.thunks.push_back({instr.src1, static_cast<std::uint16_t>(mtd.instructions.size()), "", location::IMM24, thunk_type::CLASS});
            instruction = ::construct3(::itype::VNEW, 0, dest->second.offset, 0, 0);
            break;
        }
        case keywords::keyword::IOF:
        {
            lookup_variable(dest);
            lookup_variable(src1);
            require_type(dest, 2);
            require_type(src1, 6);
            instruction = ::construct3(::itype::IOF, 0, dest->second.offset, src1->second.offset, 0);
            mtd.thunks.push_back({instr.src2, static_cast<std::uint16_t>(mtd.instructions.size()), "", location::IMM24, thunk_type::CLASS});
            break;
        }
#define thunk(var, name, loc, ttype)                                                              \
    std::size_t split_idx = instr.var.find_last_of('.');                                          \
    if (split_idx == std::string::npos)                                                           \
    {                                                                                             \
        compiling_error(name " name must b edelimited from class name by a single period ('.')"); \
    }                                                                                             \
    mtd.thunks.push_back({instr.var.substr(split_idx + 1), static_cast<std::uint16_t>(mtd.instructions.size()), instr.var.substr(0, split_idx), location::loc, thunk_type::ttype});
        case keywords::keyword::CVLLD:
        {
            lookup_variable(dest);
            lookup_variable(src1);
            require_type(dest, 2);
            require_type(src1, 6);
            instruction = ::construct3(::itype::CVLLD, 0, dest->second.offset, src1->second.offset, 0);
            thunk(src2, "Instance variable", IMM24, IVAR);
            break;
        }
        case keywords::keyword::SVLLD:
        {
            lookup_variable(dest);
            lookup_variable(src1);
            require_type(dest, 2);
            require_type(src1, 6);
            instruction = ::construct3(::itype::SVLLD, 0, dest->second.offset, src1->second.offset, 0);
            thunk(src2, "Instance variable", IMM24, IVAR);
            break;
        }
        case keywords::keyword::VLLD:
        {
            lookup_variable(dest);
            lookup_variable(src1);
            require_type(src1, 6);
            ::itype type;
            switch (dest->second.type)
            {
            case 2:
                type = ::itype::IVLLD;
                break;
            case 3:
                type = ::itype::LVLLD;
                break;
            case 4:
                type = ::itype::FVLLD;
                break;
            case 5:
                type = ::itype::DVLLD;
                break;
            case 6:
                type = ::itype::VVLLD;
                break;
            }
            instruction = ::construct3(type, 0, dest->second.offset, src1->second.offset, 0);
            thunk(src2, "Instance variable", IMM24, IVAR);
            break;
        }
        case keywords::keyword::CSTLD:
        {
            lookup_variable(dest);
            require_type(dest, 2);
            instruction = ::construct3(::itype::CSTLD, 0, dest->second.offset, 0, 0);
            thunk(src1, "Static variable", IMM32, SVAR);
            break;
        }
        case keywords::keyword::SSTLD:
        {
            lookup_variable(dest);
            require_type(dest, 2);
            instruction = ::construct3(::itype::SSTLD, 0, dest->second.offset, 0, 0);
            thunk(src1, "Static variable", IMM32, SVAR);
            break;
        }
        case keywords::keyword::STLD:
        {
            lookup_variable(dest);
            ::itype type;
            switch (dest->second.type)
            {
            case 2:
                type = ::itype::ISTLD;
                break;
            case 3:
                type = ::itype::LSTLD;
                break;
            case 4:
                type = ::itype::FSTLD;
                break;
            case 5:
                type = ::itype::DSTLD;
                break;
            case 6:
                type = ::itype::VSTLD;
                break;
            }
            instruction = ::construct3(type, 0, dest->second.offset, 0, 0);
            thunk(src1, "Static variable", IMM32, SVAR);
            break;
        }
        case keywords::keyword::CALD:
        {
            lookup_variable(dest);
            lookup_variable(src1);
            lookup_variable(src2);
            require_type(dest, 2);
            require_type(src1, 6);
            require_type(src2, 2);
            instruction = ::construct3(::itype::CVLLD, 0, dest->second.offset, src1->second.offset, src2->second.offset);
            break;
        }
        case keywords::keyword::SALD:
        {
            lookup_variable(dest);
            lookup_variable(src1);
            lookup_variable(src2);
            require_type(dest, 2);
            require_type(src1, 6);
            require_type(src2, 2);
            instruction = ::construct3(::itype::SVLLD, 0, dest->second.offset, src1->second.offset, src2->second.offset);
            break;
        }
        case keywords::keyword::ALD:
        {
            lookup_variable(dest);
            lookup_variable(src1);
            lookup_variable(src2);
            require_type(src1, 6);
            require_type(src2, 2);
            ::itype type;
            switch (dest->second.type)
            {
            case 2:
                type = ::itype::IALD;
                break;
            case 3:
                type = ::itype::LALD;
                break;
            case 4:
                type = ::itype::FALD;
                break;
            case 5:
                type = ::itype::DALD;
                break;
            case 6:
                type = ::itype::VALD;
                break;
            }
            instruction = ::construct3(type, 0, dest->second.offset, src1->second.offset, src2->second.offset);
            break;
        }
        case keywords::keyword::CVLSR:
        {
            lookup_variable(dest);
            lookup_variable(src1);
            require_type(dest, 6);
            require_type(src1, 2);
            instruction = ::construct3(::itype::CVLSR, 0, dest->second.offset, src1->second.offset, 0);
            thunk(src2, "Instance variable", IMM24, IVAR);
            break;
        }
        case keywords::keyword::SVLSR:
        {
            lookup_variable(dest);
            lookup_variable(src1);
            require_type(dest, 6);
            require_type(src1, 2);
            instruction = ::construct3(::itype::SVLSR, 0, dest->second.offset, src1->second.offset, 0);
            thunk(src2, "Instance variable", IMM24, IVAR);
            break;
        }
        case keywords::keyword::VLSR:
        {
            lookup_variable(dest);
            lookup_variable(src1);
            require_type(dest, 6);
            ::itype type;
            switch (src1->second.type)
            {
            case 2:
                type = ::itype::IVLSR;
                break;
            case 3:
                type = ::itype::LVLSR;
                break;
            case 4:
                type = ::itype::FVLSR;
                break;
            case 5:
                type = ::itype::DVLSR;
                break;
            case 6:
                type = ::itype::VVLSR;
                break;
            }
            instruction = ::construct3(type, 0, dest->second.offset, src1->second.offset, 0);
            thunk(src2, "Instance variable", IMM24, IVAR);
            break;
        }
        case keywords::keyword::CSTSR:
        {
            lookup_variable(src1);
            require_type(src1, 2);
            instruction = ::construct32(::itype::CSTSR, 0, src1->second.offset, 0);
            thunk(dest, "Static variable", IMM32, SVAR);
            break;
        }
        case keywords::keyword::SSTSR:
        {
            lookup_variable(src1);
            require_type(src1, 2);
            instruction = ::construct32(::itype::SSTSR, 0, src1->second.offset, 0);
            thunk(dest, "Static variable", IMM32, SVAR);
            break;
        }
        case keywords::keyword::STSR:
        {
            lookup_variable(src1);
            ::itype type;
            switch (src1->second.type)
            {
            case 2:
                type = ::itype::ISTSR;
                break;
            case 3:
                type = ::itype::LSTSR;
                break;
            case 4:
                type = ::itype::FSTSR;
                break;
            case 5:
                type = ::itype::DSTSR;
                break;
            case 6:
                type = ::itype::VSTSR;
                break;
            }
            instruction = ::construct32(type, 0, src1->second.offset, 0);
            thunk(dest, "Static variable", IMM32, SVAR);
            break;
        }
        case keywords::keyword::CASR:
        {
            lookup_variable(dest);
            lookup_variable(src1);
            lookup_variable(src2);
            require_type(dest, 6);
            require_type(src1, 2);
            require_type(src2, 2);
            instruction = ::construct3(::itype::CASR, 0, dest->second.offset, src1->second.offset, src2->second.offset);
            break;
        }
        case keywords::keyword::SASR:
        {
            lookup_variable(dest);
            lookup_variable(src1);
            lookup_variable(src2);
            require_type(dest, 6);
            require_type(src1, 2);
            require_type(src2, 2);
            instruction = ::construct3(::itype::SASR, 0, dest->second.offset, src1->second.offset, src2->second.offset);
            break;
        }
        case keywords::keyword::ASR:
        {
            lookup_variable(dest);
            lookup_variable(src1);
            lookup_variable(src2);
            require_type(dest, 6);
            require_type(src2, 2);
            ::itype type;
            switch (src1->second.type)
            {
            case 2:
                type = ::itype::IVLSR;
                break;
            case 3:
                type = ::itype::LVLSR;
                break;
            case 4:
                type = ::itype::FVLSR;
                break;
            case 5:
                type = ::itype::DVLSR;
                break;
            case 6:
                type = ::itype::VVLSR;
                break;
            }
            instruction = ::construct3(type, 0, dest->second.offset, src1->second.offset, src2->second.offset);
            break;
        }
        case keywords::keyword::VINV:
        {
            lookup_variable(dest);
            lookup_variable(src1);
            require_type(src1, 6);
            instruction = ::construct24(::itype::VINV, dest->second.offset, src1->second.offset, 0);
            thunk(src2, "Method", IMM24, METHOD);
            break;
        }
        case keywords::keyword::IINV:
        {
            lookup_variable(dest);
            lookup_variable(src1);
            require_type(src1, 6);
            instruction = ::construct24(::itype::IINV, dest->second.offset, src1->second.offset, 0);
            thunk(src2, "Method", IMM24, METHOD);
            break;
        }
        case keywords::keyword::SINV:
        {
            lookup_variable(dest);
            instruction = ::construct32(::itype::SINV, 0, dest->second.offset, 0);
            thunk(src1, "Method", IMM32, METHOD);
            break;
        }
        case keywords::keyword::LBL:
        case keywords::keyword::DEF:
            continue;
        case keywords::keyword::BCMP:
        case keywords::keyword::BADR:
        case keywords::keyword::NOP:
        default:
            compiling_error("Unsupported keyword type in procedure");
        }
        mtd.instructions.push_back(instruction);
    }
    return mtd;
}