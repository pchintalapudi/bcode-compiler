#ifndef COMPILER_COMPILER
#define COMPILER_COMPILER

#include <cstdint>
#include <variant>
#include <vector>

#include "../parser/parser.h"

namespace oops_bcode_compiler
{
    namespace compiler
    {
        enum class thunk_type
        {
            CLASS,
            METHOD,
            SVAR,
            IVAR
        };

        enum class location
        {
            DEST,
            SRC1,
            SRC2,
            IMM24,
            IMM32
        };

        struct thunk
        {
            std::string name;
            std::uint32_t instruction_idx;
            std::string class_name;
            location rewrite_location;
            thunk_type type;
        };

        struct method
        {
            std::vector<std::uint64_t> instructions;
            std::vector<thunk> thunks;
            std::vector<std::uint16_t> handle_map;
            std::uint16_t stack_size;
            std::uint8_t return_type;
            std::uint8_t method_type;
            std::vector<std::uint8_t> arg_types;
            std::uint64_t size;
        };

        std::variant<method, std::string> compile(const oops_bcode_compiler::parsing::cls::procedure &procedure, std::stringstream& error_builder);
    } // namespace compiler
} // namespace oops_bcode_compiler

#endif /* COMPILER_COMPILER */
