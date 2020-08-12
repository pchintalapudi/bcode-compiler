#ifndef INSTRUCTIONS_KEYWORDS
#define INSTRUCTIONS_KEYWORDS
#include <array>
#include <string>
#include <unordered_map>

namespace oops_bcode_compiler
{
    namespace keywords
    {
        enum class keyword
        {
            NOP,
            ADD,
            SUB,
            MUL,
            DIV,
            DIVU,
            ADDI,
            SUBI,
            MULI,
            DIVI,
            DIVUI,
            NEG,
            LUI,
            LLI,
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
            AND,
            OR,
            XOR,
            SLL,
            SRL,
            SRA,
            ANDI,
            ORI,
            XORI,
            SLLI,
            SRLI,
            SRAI,
            BGE,
            BLT,
            BLE,
            BGT,
            BEQ,
            BNEQ,
            BGEI,
            BLTI,
            BLEI,
            BGTI,
            BEQI,
            BNEQI,
            BCMP,
            BADR,
            BU,
            VLLD,
            VLSR,
            ALD,
            ASR,
            STLD,
            STSR,
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
            RET,
            EXC,
            IMP,
            IVAR,
            SVAR,
            DEF,
            PROC,
            CLZ,
            __COUNT__
        };

        std::array<std::string, static_cast<unsigned>(keyword::__COUNT__)> generate_keyword_to_string()
        {
            std::array<std::string, static_cast<unsigned>(keyword::__COUNT__)> ret;
            std::fill(ret.begin(), ret.end(), "UNKNOWN");
#define stringize(enumeration) ret[static_cast<unsigned>(keyword::enumeration)] = #enumeration
            stringize(NOP);
            stringize(ADD);
            stringize(SUB);
            stringize(MUL);
            stringize(DIV);
            stringize(DIVU);
            stringize(ADDI);
            stringize(SUBI);
            stringize(MULI);
            stringize(DIVI);
            stringize(DIVUI);
            stringize(NEG);
            stringize(LUI);
            stringize(LLI);
            stringize(LNL);
            stringize(ICSTL);
            stringize(ICSTF);
            stringize(ICSTD);
            stringize(LCSTI);
            stringize(LCSTF);
            stringize(LCSTD);
            stringize(FCSTI);
            stringize(FCSTL);
            stringize(FCSTD);
            stringize(DCSTI);
            stringize(DCSTL);
            stringize(DCSTF);
            stringize(AND);
            stringize(OR);
            stringize(XOR);
            stringize(SLL);
            stringize(SRL);
            stringize(SRA);
            stringize(ANDI);
            stringize(ORI);
            stringize(XORI);
            stringize(SLLI);
            stringize(SRLI);
            stringize(SRAI);
            stringize(BGE);
            stringize(BLT);
            stringize(BLE);
            stringize(BGT);
            stringize(BEQ);
            stringize(BNEQ);
            stringize(BGEI);
            stringize(BLTI);
            stringize(BLEI);
            stringize(BGTI);
            stringize(BEQI);
            stringize(BNEQI);
            stringize(BCMP);
            stringize(BADR);
            stringize(BU);
            stringize(VLLD);
            stringize(VLSR);
            stringize(ALD);
            stringize(ASR);
            stringize(STLD);
            stringize(STSR);
            stringize(VNEW);
            stringize(CANEW);
            stringize(SANEW);
            stringize(IANEW);
            stringize(LANEW);
            stringize(FANEW);
            stringize(DANEW);
            stringize(VANEW);
            stringize(IOF);
            stringize(VINV);
            stringize(SINV);
            stringize(IINV);
            stringize(RET);
            stringize(EXC);
            stringize(IMP);
            stringize(IVAR);
            stringize(SVAR);
            stringize(DEF);
            stringize(PROC);
            stringize(CLZ);
#undef put
            return ret;
        }

        std::array<std::string, static_cast<unsigned>(keyword::__COUNT__)> keyword_to_string = generate_keyword_to_string();

        std::unordered_map<std::string, keyword> generate_string_to_keyword()
        {
            std::unordered_map<std::string, keyword> ret;
            for (auto i = 0u; i < keyword_to_string.size(); i++)
            {
                ret[keyword_to_string[i]] = static_cast<keyword>(i);
            }
            return ret;
        }
        
        std::unordered_map<std::string, keyword> string_to_keywords = generate_string_to_keyword();

    } // namespace keywords
} // namespace oops_bcode_compiler
#endif /* INSTRUCTIONS_KEYWORDS */
