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
            //Skip
            NOP,
            //Done
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
            LI,
            CST,
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
            //Skip
            BCMP,
            BADR,
            //Done
            BU,
            //TODO
            CVLLD,
            SVLLD,
            VLLD,
            CVLSR,
            SVLSR,
            VLSR,
            CALD,
            SALD,
            ALD,
            CASR,
            SASR,
            ASR,
            CSTLD,
            SSTLD,
            STLD,
            CSTSR,
            SSTSR,
            STSR,
            //Done
            VNEW,
            ANEW,
            IOF,
            //TODO
            VINV,
            SINV,
            IINV,
            RET,
            //Skip
            EXC,
            IMP,
            IVAR,
            SVAR,
            //Done
            DEF,
            PASS,
            //Skip
            ARG,
            //Done
            LBL,
            //Skip
            PROC,
            SPROC,
            EPROC,
            EXT,
            IMPL,
            CLZ,
            __COUNT__
        };

        inline std::array<std::string, static_cast<unsigned>(keyword::__COUNT__)> generate_keyword_to_string()
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
            stringize(LI);
            stringize(CST);
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
            stringize(ANEW);
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
            stringize(PASS);
            stringize(ARG);
            stringize(LBL);
            stringize(PROC);
            stringize(SPROC);
            stringize(EPROC);
            stringize(EXT);
            stringize(IMPL);
            stringize(CLZ);
#undef put
            return ret;
        }

        inline std::array<std::string, static_cast<unsigned>(keyword::__COUNT__)> keyword_to_string = generate_keyword_to_string();

        inline std::unordered_map<std::string, keyword> generate_string_to_keyword()
        {
            std::unordered_map<std::string, keyword> ret;
            for (auto i = 0u; i < keyword_to_string.size(); i++)
            {
                ret[keyword_to_string[i]] = static_cast<keyword>(i);
            }
            return ret;
        }

        inline std::unordered_map<std::string, keyword> string_to_keywords = generate_string_to_keyword();

    } // namespace keywords
} // namespace oops_bcode_compiler
#endif /* INSTRUCTIONS_KEYWORDS */
