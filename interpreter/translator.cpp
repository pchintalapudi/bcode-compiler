#include "translator.h"

#include <algorithm>
#include <numeric>
#include <string>
#include <vector>

#include "../platform_specific/files.h"
#include "../utils/puns.h"

using namespace oops_bcode_compiler::transformer;

namespace
{
    std::uint64_t string_pool_size(oops_bcode_compiler::parsing::cls &cls)
    {
        return 0;
    }

    struct method
    {
        std::vector<std::uint64_t> instructions;
        std::uint64_t name_offset;
        std::vector<std::uint16_t> handle_map;
        std::uint16_t stack_size;
        std::uint8_t return_type;
        std::uint8_t method_type;
        std::uint16_t arg_count;
        std::vector<std::uint8_t> arg_types;
        std::uint64_t size;
    };

    method compile(const oops_bcode_compiler::parsing::cls::procedure &procedure)
    {
        ::method mtd;
        return mtd;
    }
} // namespace

bool oops_bcode_compiler::transformer::write(oops_bcode_compiler::parsing::cls cls)
{
    std::uint64_t classes_offset, methods_offset, statics_offset, instances_offset, bytecode_offset, string_offset;
    classes_offset = 6 * sizeof(std::uint64_t);
    methods_offset = classes_offset + sizeof(std::uint32_t) * 2 + sizeof(std::uint64_t) * cls.imports.size();
    statics_offset = methods_offset + sizeof(std::uint32_t) * 2 + (sizeof(std::uint32_t) * 2 + sizeof(std::uint64_t)) * cls.methods.size();
    instances_offset = statics_offset + sizeof(std::uint32_t) * 2 + (sizeof(std::uint32_t) * 2 + sizeof(std::uint64_t)) * cls.static_variables.size();
    bytecode_offset = instances_offset + sizeof(std::uint32_t) * 2 + (sizeof(std::uint32_t) * 2 + sizeof(std::uint64_t)) * cls.instance_variables.size();
    std::vector<::method> compiled_methods;
    std::transform(cls.self_methods.begin(), cls.self_methods.end(), std::back_inserter(compiled_methods), ::compile);
    string_offset = bytecode_offset + sizeof(std::uint64_t) + std::accumulate(compiled_methods.begin(), compiled_methods.end(), static_cast<std::uint64_t>(0), [](auto sum, auto method) { return sum + method.size; });
    std::uint64_t cls_size = string_offset + string_pool_size(cls);
    if (auto maybe_cls = platform::create_class_file(cls.imports[6], cls_size))
    {
        utils::pun_write(maybe_cls->mmapped_file, classes_offset);
        utils::pun_write(maybe_cls->mmapped_file, methods_offset);
        utils::pun_write(maybe_cls->mmapped_file, statics_offset);
        utils::pun_write(maybe_cls->mmapped_file, instances_offset);
        utils::pun_write(maybe_cls->mmapped_file, bytecode_offset);
        utils::pun_write(maybe_cls->mmapped_file, string_offset);
        char *base_head = maybe_cls->mmapped_file + classes_offset;
        utils::pun_write<std::uint32_t>(base_head, cls.imports.size());
        utils::pun_write<std::uint32_t>(base_head + sizeof(std::uint32_t), cls.implement_count);
        base_head += sizeof(std::uint32_t) * 2 + sizeof(std::uint64_t) * 6;
        std::uint64_t current_string_offset = string_offset;
        for (auto imp = cls.imports.begin() + 6; imp != cls.imports.end(); ++imp)
        {
            utils::pun_write(base_head, current_string_offset);
            base_head += sizeof(current_string_offset);
            utils::pun_write(maybe_cls->mmapped_file + current_string_offset, static_cast<std::uint32_t>(imp->size()));
            std::memcpy(maybe_cls->mmapped_file + sizeof(std::uint32_t) + current_string_offset, imp->c_str(), imp->size());
            current_string_offset += imp->size() + sizeof(std::uint32_t);
        }
        utils::pun_write<std::uint32_t>(base_head, cls.methods.size());
        utils::pun_write<std::uint32_t>(base_head + sizeof(std::uint32_t), cls.static_method_count);
        base_head += sizeof(std::uint32_t) * 2;
        platform::close_file_mapping(*maybe_cls);
        return true;
    }
    return false;
}