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
            std::size_t implement_count;
            std::size_t static_method_count;
            std::vector<std::string> imports;
            struct variable
            {
                std::string host_name;
                std::string name;
            };
            std::vector<variable> static_variables, instance_variables, self_statics, self_instances;
            typedef variable method;
            std::vector<method> methods;
            struct instruction {
                std::string dest;
                std::string src1;
                std::string src2;
                keywords::keyword itype;
            };
            struct procedure {
                std::string return_type_name;
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
