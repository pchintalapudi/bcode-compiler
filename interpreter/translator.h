#ifndef INTERPRETER_TRANSLATOR
#define INTERPRETER_TRANSLATOR

#include "../parser/parser.h"

namespace oops_bcode_compiler
{
    namespace transformer
    {
        std::optional<std::string> write(parsing::cls clz, std::string build_path);
    } // namespace transformer
} // namespace oops_bcode_compiler
#endif /* INTERPRETER_TRANSLATOR */
