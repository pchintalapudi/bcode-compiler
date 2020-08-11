#include "lexer.h"

#include <cctype>

using namespace oops_bcode_compiler::lexer;

lexer::lexer(std::string filename) {
    if (auto mapping = platform::open_class_file_mapping(filename)) {
        this->mapping = *mapping;
    } else {
        this->mapping.mmapped_file = nullptr;
    }
}

lexer::~lexer() {
    if (this->mapping.mmapped_file) {
        platform::close_file_mapping(this->mapping);
    }
}


const std::vector<line>& lexer::lex() const {
    if (this->cache) return *this->cache;
    std::vector<line> lexed;
    char* end = this->mapping.mmapped_file + this->mapping.file_size;
    char* current = this->mapping.mmapped_file;
    
    return *(this->cache = lexed);
}