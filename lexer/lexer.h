#ifndef LEXER_LEXER
#define LEXER_LEXER
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "../platform_specific/files.h"

namespace oops_bcode_compiler {
    namespace lexer {

        typedef std::variant<void> line;

        class lexer {
            private:
            platform::file_mapping mapping;
            mutable std::optional<std::vector<line>> cache;
            public:
            lexer(std::string filename);
            ~lexer();

            const std::vector<line>& lex() const;
        };
    }
}
#endif /* LEXER_LEXER */
