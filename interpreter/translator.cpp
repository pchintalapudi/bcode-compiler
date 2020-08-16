#include "translator.h"

#include <algorithm>
#include <numeric>
#include <string>
#include <vector>

#include "../platform_specific/files.h"
#include "../utils/puns.h"
#include "../utils/hashing.h"

using namespace oops_bcode_compiler::transformer;

namespace
{
    std::uint64_t string_pool_size(oops_bcode_compiler::parsing::cls &cls)
    {
        return 0;
    }

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
        std::uint32_t class_index;
        location rewrite_location;
        thunk_type type;
    };

    struct method
    {
        std::vector<std::uint64_t> instructions;
        std::vector<thunk> thunks;
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

    std::size_t dethunk(thunk &t, std::size_t thunked, std::uint64_t value)
    {
        switch (t.rewrite_location)
        {
        case location::DEST:
            return thunked | value;
        case location::IMM32:
        case location::SRC1:
            return thunked | value << sizeof(std::uint16_t) * CHAR_BIT;
        case location::IMM24:
        case location::SRC2:
            return thunked | value << sizeof(std::uint16_t) * 2 * CHAR_BIT;
        }
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
        std::unordered_map<std::string, std::uint32_t> class_indexes = {{"char", 0}, {"short", 1}, {"int", 2}, {"long", 3}, {"float", 4}, {"double", 5}};
        for (auto imp = cls.imports.begin() + 6; imp != cls.imports.end(); ++imp)
        {
            class_indexes[*imp] = imp - cls.imports.begin();
            utils::pun_write(base_head, current_string_offset);
            base_head += sizeof(current_string_offset);
            utils::pun_write(maybe_cls->mmapped_file + current_string_offset, static_cast<std::uint32_t>(imp->size()));
            std::memcpy(maybe_cls->mmapped_file + sizeof(std::uint32_t) + current_string_offset, imp->c_str(), imp->size());
            current_string_offset += imp->size() + sizeof(std::uint32_t);
        }
        utils::pun_write<std::uint32_t>(base_head, cls.methods.size());
        utils::pun_write<std::uint32_t>(base_head + sizeof(std::uint32_t), cls.static_method_count);
        base_head += sizeof(std::uint32_t) * 2;
        std::unordered_map<std::pair<std::string, std::uint32_t>, std::uint32_t, utils::container_hasher<std::pair>> method_indexes;
        auto bmethod = compiled_methods.begin();
        for (auto method = cls.methods.begin(); method != cls.methods.end(); ++method)
        {
            if ((method_indexes[{method->name, class_indexes[method->host_name]}] = method - cls.methods.begin()) == 6) {
                bmethod->name_offset = current_string_offset;
            }
            utils::pun_write(base_head, class_indexes[method->host_name]);
            utils::pun_write<std::uint32_t>(base_head + sizeof(std::uint32_t), 0);
            utils::pun_write(base_head + sizeof(std::uint32_t) * 2, current_string_offset);
            base_head += sizeof(std::uint32_t) * 2 + sizeof(current_string_offset);
            utils::pun_write(maybe_cls->mmapped_file + current_string_offset, static_cast<std::uint32_t>(method->name.size()));
            std::memcpy(maybe_cls->mmapped_file + sizeof(std::uint32_t) + current_string_offset, method->name.c_str(), method->name.size());
            current_string_offset += method->name.size() + sizeof(std::uint32_t);
        }
        std::unordered_map<std::pair<std::string, std::uint32_t>, std::uint32_t, utils::container_hasher<std::pair>> static_indexes;
        for (auto svar = cls.static_variables.begin(); svar != cls.static_variables.end(); ++svar)
        {
            static_indexes[{svar->name, class_indexes[svar->host_name]}] = svar - cls.static_variables.begin();
            utils::pun_write(base_head, class_indexes[svar->host_name]);
            utils::pun_write<std::uint32_t>(base_head + sizeof(std::uint32_t), 0);
            utils::pun_write(base_head + sizeof(std::uint32_t) * 2, current_string_offset);
            base_head += sizeof(std::uint32_t) * 2 + sizeof(current_string_offset);
            utils::pun_write(maybe_cls->mmapped_file + current_string_offset, static_cast<std::uint32_t>(svar->name.size()));
            std::memcpy(maybe_cls->mmapped_file + sizeof(std::uint32_t) + current_string_offset, svar->name.c_str(), svar->name.size());
            current_string_offset += svar->name.size() + sizeof(std::uint32_t);
        }
        std::unordered_map<std::pair<std::string, std::uint32_t>, std::uint32_t, utils::container_hasher<std::pair>> instance_indexes;
        for (auto ivar = cls.instance_variables.begin(); ivar != cls.instance_variables.end(); ++ivar)
        {
            instance_indexes[{ivar->name, class_indexes[ivar->host_name]}] = ivar - cls.instance_variables.begin();
            utils::pun_write(base_head, class_indexes[ivar->host_name]);
            utils::pun_write<std::uint32_t>(base_head + sizeof(std::uint32_t), 0);
            utils::pun_write(base_head + sizeof(std::uint32_t) * 2, current_string_offset);
            base_head += sizeof(std::uint32_t) * 2 + sizeof(current_string_offset);
            utils::pun_write(maybe_cls->mmapped_file + current_string_offset, static_cast<std::uint32_t>(ivar->name.size()));
            std::memcpy(maybe_cls->mmapped_file + sizeof(std::uint32_t) + current_string_offset, ivar->name.c_str(), ivar->name.size());
            current_string_offset += ivar->name.size() + sizeof(std::uint32_t);
        }
        utils::pun_write()
        for (auto &method : compiled_methods)
        {
            for (auto &thunk : method.thunks)
            {
                switch (thunk.type)
                {
                case thunk_type::CLASS:
                {
                    method.instructions[thunk.instruction_idx] = dethunk(thunk, method.instructions[thunk.instruction_idx], class_indexes[thunk.name]);
                    break;
                }
                case thunk_type::METHOD:
                {
                    method.instructions[thunk.instruction_idx] = dethunk(thunk, method.instructions[thunk.instruction_idx], method_indexes[{thunk.name, thunk.class_index}]);
                    break;
                }
                case thunk_type::IVAR:
                {
                    method.instructions[thunk.instruction_idx] = dethunk(thunk, method.instructions[thunk.instruction_idx], instance_indexes[{thunk.name, thunk.class_index}]);
                    break;
                }
                case thunk_type::SVAR:
                {
                    method.instructions[thunk.instruction_idx] = dethunk(thunk, method.instructions[thunk.instruction_idx], static_indexes[{thunk.name, thunk.class_index}]);
                    break;
                }
                }
            }

        }
        platform::close_file_mapping(*maybe_cls);
        return true;
    }
    return false;
}