#include "translator.h"

#include <string>
#include <vector>

#include "../platform_specific/files.h"
#include "../utils/puns.h"

using namespace oops_bcode_compiler::transformer;

bool oops_bcode_compiler::transformer::write(oops_bcode_compiler::parsing::cls cls)
{
    if (auto maybe_cls = platform::create_class_file(cls.name))
    {

        platform::close_file_mapping(*maybe_cls);
    }
    return false;
}