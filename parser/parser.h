#ifndef LEXER_LEXER
#define LEXER_LEXER
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "../platform_specific/files.h"
#include "../instructions/keywords.h"

namespace oops_bcode_compiler
{
    namespace parsing
    {

        struct cls
        {
            std::string name;
            std::vector<std::string> imports;
            struct variable
            {
                std::size_t import_index;
                std::string name;
            };
            std::vector<variable> static_variables, instance_variables;
            typedef variable method;
            std::vector<method> methods;
            struct instruction {
                std::string dest;
                std::string src1;
                std::string src2;
                keywords::keyword itype;
            };
            struct procedure {
                std::size_t return_import_index;
                std::vector<variable> parameters;
                std::vector<instruction> instructions;
            };
            std::vector<procedure> self_methods;
        };

        class parser
        {
        private:
            platform::file_mapping mapping;

        public:
            parser(std::string filename);
            std::variant<cls, std::string> parse();
            ~parser();
        };
    } // namespace lexer
} // namespace oops_bcode_compiler
#endif /* LEXER_LEXER */
