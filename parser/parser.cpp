#include "parser.h"

#include <cctype>

using namespace oops_bcode_compiler::lexer;

parser::parser(std::string filename) {
    if (auto mapping = platform::open_class_file_mapping(filename)) {
        this->mapping = *mapping;
    } else {
        this->mapping.mmapped_file = nullptr;
    }
}

parser::~parser() {
    if (this->mapping.mmapped_file) {
        platform::close_file_mapping(this->mapping);
    }
}