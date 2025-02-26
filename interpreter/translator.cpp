#include "translator.h"

#include <algorithm>
#include <numeric>
#include <string>
#include <sstream>
#include <vector>

#include "../platform_specific/files.h"
#include "../utils/puns.h"
#include "../utils/hashing.h"
#include "../compiler/compiler.h"
#include "../debug/logs.h"

using namespace oops_bcode_compiler::transformer;
using namespace oops_bcode_compiler::debug;

namespace
{

    std::size_t round_off(std::size_t in, std::size_t align = sizeof(std::uint32_t))
    {
        return (in + align - 1) & ~(align - 1);
    }
    std::uint64_t string_pool_size(oops_bcode_compiler::parsing::cls &cls)
    {
        std::uint64_t size = 0;
        size += std::accumulate(cls.imports.begin() + 6, cls.imports.end(), static_cast<std::size_t>(0), [](std::size_t sum, const auto &imp) { return sum + ::round_off(imp.name.length() + sizeof(std::uint32_t)); });
        size += std::accumulate(cls.methods.begin(), cls.methods.end(), static_cast<std::size_t>(0), [](std::size_t sum, const oops_bcode_compiler::parsing::cls::method &mtd) { return sum + ::round_off(mtd.name.length() + sizeof(std::uint32_t) * 2); });
        size += std::accumulate(cls.static_variables.begin(), cls.static_variables.end(), static_cast<std::size_t>(0), [](std::size_t sum, const oops_bcode_compiler::parsing::cls::variable &svar) { return sum + ::round_off(svar.name.length() + sizeof(std::uint32_t) * 2); });
        size += std::accumulate(cls.instance_variables.begin(), cls.instance_variables.end(), static_cast<std::size_t>(0), [](std::size_t sum, const oops_bcode_compiler::parsing::cls::variable &ivar) { return sum + ::round_off(ivar.name.length() + sizeof(std::uint32_t) * 2); });
        return size;
    }

    std::size_t dethunk(oops_bcode_compiler::compiler::thunk &t, std::size_t thunked, std::uint64_t value)
    {
        switch (t.rewrite_location)
        {
        case oops_bcode_compiler::compiler::location::DEST:
            return thunked | value;
        case oops_bcode_compiler::compiler::location::IMM32:
        case oops_bcode_compiler::compiler::location::SRC1:
            return thunked | value << sizeof(std::uint16_t) * CHAR_BIT;
        case oops_bcode_compiler::compiler::location::IMM24:
        case oops_bcode_compiler::compiler::location::SRC2:
            return thunked | value << sizeof(std::uint16_t) * 2 * CHAR_BIT;
        }
    }
} // namespace

std::vector<std::string> oops_bcode_compiler::transformer::write(oops_bcode_compiler::parsing::cls cls, std::string build_path)
{
    std::vector<std::string> errors;
    std::stringstream error_builder;
    std::uint64_t classes_offset, methods_offset, statics_offset, instances_offset, bytecode_offset, string_offset;
    classes_offset = 6 * sizeof(std::uint64_t);
    methods_offset = classes_offset + sizeof(std::uint32_t) * 2 + sizeof(std::uint64_t) * (cls.imports.size() - 6);
    statics_offset = methods_offset + sizeof(std::uint32_t) * 2 + (sizeof(std::uint32_t) * 2 + sizeof(std::uint64_t)) * cls.methods.size();
    instances_offset = statics_offset + sizeof(std::uint32_t) * 2 + (sizeof(std::uint32_t) * 2 + sizeof(std::uint64_t)) * cls.static_variables.size();
    bytecode_offset = instances_offset + sizeof(std::uint32_t) * 2 + (sizeof(std::uint32_t) * 2 + sizeof(std::uint64_t)) * cls.instance_variables.size();
    std::vector<compiler::method> compiled_methods;
    for (auto &proc : cls.self_methods)
    {
        auto maybe_method = compiler::compile(proc);
        if (std::holds_alternative<compiler::method>(maybe_method))
        {
            compiled_methods.push_back(std::get<compiler::method>(maybe_method));
        }
        else
        {
            auto &compile_errors = std::get<std::vector<std::string>>(maybe_method);
            std::copy(compile_errors.begin(), compile_errors.end(), std::back_inserter(errors));
        }
    }
    string_offset = bytecode_offset + sizeof(std::uint64_t) + std::accumulate(compiled_methods.begin(), compiled_methods.end(), static_cast<std::uint64_t>(0), [](auto sum, auto method) { return sum + method.size; });
    logger.builder(logging::level::debug) << "classes_offset " << classes_offset << logging::logbuilder::end;
    logger.builder(logging::level::debug) << "methods_offset " << methods_offset << logging::logbuilder::end;
    logger.builder(logging::level::debug) << "statics_offset " << statics_offset << logging::logbuilder::end;
    logger.builder(logging::level::debug) << "instances_offset " << instances_offset << logging::logbuilder::end;
    logger.builder(logging::level::debug) << "bytecode_offset " << bytecode_offset << logging::logbuilder::end;
    logger.builder(logging::level::debug) << "string_offset " << string_offset << logging::logbuilder::end;
    std::uint64_t cls_size = string_offset + string_pool_size(cls);
    if (auto maybe_cls = platform::create_class_file(cls.imports[6].name, cls_size, build_path))
    {
        utils::pun_write(maybe_cls->mmapped_file, classes_offset);
        utils::pun_write(maybe_cls->mmapped_file + sizeof(std::uint64_t), methods_offset);
        utils::pun_write(maybe_cls->mmapped_file + sizeof(std::uint64_t) * 2, statics_offset);
        utils::pun_write(maybe_cls->mmapped_file + sizeof(std::uint64_t) * 3, instances_offset);
        utils::pun_write(maybe_cls->mmapped_file + sizeof(std::uint64_t) * 4, bytecode_offset);
        utils::pun_write(maybe_cls->mmapped_file + sizeof(std::uint64_t) * 5, string_offset);
        char *base_head = maybe_cls->mmapped_file + classes_offset;
        utils::pun_write<std::uint32_t>(base_head, cls.imports.size() - 6);
        utils::pun_write<std::uint32_t>(base_head + sizeof(std::uint32_t), cls.implement_count);
        base_head += sizeof(std::uint32_t) * 2;
        std::uint64_t current_string_offset = string_offset;
        std::unordered_map<std::string, std::uint32_t> class_indexes = {{"char", 0}, {"short", 1}, {"int", 2}, {"long", 3}, {"float", 4}, {"double", 5}};
        for (auto imp = cls.imports.begin() + 6; imp != cls.imports.end(); ++imp)
        {
            if (class_indexes.find(imp->name) != class_indexes.end())
            {
                error_builder << "Import " << imp->name << " was imported twice at line " << imp->line_number << " and column " << imp->column_number;
                errors.push_back(error_builder.str());
                logger.debug(error_builder.str());
                error_builder.clear();
                continue;
            }
            class_indexes[imp->name] = imp - cls.imports.begin();
            utils::pun_write(base_head, current_string_offset);
            logger.builder(logging::level::debug) << "Import name: " << imp->name << logging::logbuilder::end;
            logger.builder(logging::level::debug) << "Base head: " << static_cast<std::uintptr_t>(base_head - maybe_cls->mmapped_file) << logging::logbuilder::end;
            logger.builder(logging::level::debug) << "Current string offset: " << current_string_offset << logging::logbuilder::end;
            base_head += sizeof(current_string_offset);
            utils::pun_write(maybe_cls->mmapped_file + current_string_offset, static_cast<std::uint32_t>(imp->name.size()));
            std::memcpy(maybe_cls->mmapped_file + sizeof(std::uint32_t) + current_string_offset, imp->name.c_str(), imp->name.size());
            current_string_offset += ::round_off(imp->name.size() + sizeof(std::uint32_t));
        }
        utils::pun_write<std::uint32_t>(base_head, cls.methods.size());
        utils::pun_write<std::uint32_t>(base_head + sizeof(std::uint32_t), cls.static_method_count);
        base_head += sizeof(std::uint32_t) * 2;
        std::unordered_map<std::pair<std::string, std::uint32_t>, std::uint32_t, utils::container_hasher<std::pair>> method_indexes;
        for (auto method = cls.methods.begin(); method != cls.methods.end(); ++method)
        {
            if (auto cidx = class_indexes.find(method->host_name); cidx != class_indexes.end())
            {
                utils::pun_write(base_head, cidx->second);
                method_indexes[{method->name, cidx->second}] = method - cls.methods.begin();
            }
            else
            {
                error_builder << "Unable to find method import class" << method->host_name << "!";
                errors.push_back(error_builder.str());
                logger.debug(error_builder.str());
                error_builder.clear();
                continue;
            }
            utils::pun_write<std::uint32_t>(base_head + sizeof(std::uint32_t), 0);
            utils::pun_write(base_head + sizeof(std::uint32_t) * 2, current_string_offset);
            logger.builder(logging::level::debug) << "Method name: " << method->name << logging::logbuilder::end;
            logger.builder(logging::level::debug) << "Method host: " << method->host_name << logging::logbuilder::end;
            logger.builder(logging::level::debug) << "Base head: " << static_cast<std::uintptr_t>(base_head - maybe_cls->mmapped_file) << logging::logbuilder::end;
            logger.builder(logging::level::debug) << "Current string offset: " << current_string_offset << logging::logbuilder::end;
            base_head += sizeof(std::uint32_t) * 2 + sizeof(current_string_offset);
            utils::pun_write(maybe_cls->mmapped_file + current_string_offset, static_cast<std::uint32_t>(method->name.size()));
            std::memcpy(maybe_cls->mmapped_file + sizeof(std::uint32_t) + current_string_offset, method->name.c_str(), method->name.size());
            current_string_offset += ::round_off(method->name.size() + sizeof(std::uint32_t));
        }
        utils::pun_write<std::uint32_t>(base_head, cls.static_variables.size());
        utils::pun_write<std::uint32_t>(base_head + sizeof(std::uint32_t), 0);
        base_head += sizeof(std::uint32_t) * 2;
        std::unordered_map<std::pair<std::string, std::uint32_t>, std::uint32_t, utils::container_hasher<std::pair>> static_indexes;
        for (auto svar = cls.static_variables.begin(); svar != cls.static_variables.end(); ++svar)
        {
            if (auto cidx = class_indexes.find(svar->host_name); cidx != class_indexes.end())
            {
                static_indexes[{svar->name, cidx->second}] = svar - cls.static_variables.begin();
                utils::pun_write(base_head, cidx->second);
            }
            else
            {
                error_builder << "Unable to find static variable import " << svar->host_name << "!";
                errors.push_back(error_builder.str());
                logger.debug(error_builder.str());
                error_builder.clear();
                continue;
            }
            utils::pun_write<std::uint32_t>(base_head + sizeof(std::uint32_t), 0);
            utils::pun_write(base_head + sizeof(std::uint32_t) * 2, current_string_offset);
            logger.builder(logging::level::debug) << "Static name: " << svar->name << logging::logbuilder::end;
            logger.builder(logging::level::debug) << "Static host: " << svar->host_name << logging::logbuilder::end;
            logger.builder(logging::level::debug) << "Base head: " << static_cast<std::uintptr_t>(base_head - maybe_cls->mmapped_file) << logging::logbuilder::end;
            logger.builder(logging::level::debug) << "Current string offset: " << current_string_offset << logging::logbuilder::end;
            base_head += sizeof(std::uint32_t) * 2 + sizeof(current_string_offset);
            utils::pun_write(maybe_cls->mmapped_file + current_string_offset, static_cast<std::uint32_t>(svar->name.size()));
            std::memcpy(maybe_cls->mmapped_file + sizeof(std::uint32_t) + current_string_offset, svar->name.c_str(), svar->name.size());
            current_string_offset += ::round_off(svar->name.size() + sizeof(std::uint32_t));
        }
        utils::pun_write<std::uint32_t>(base_head, cls.instance_variables.size());
        utils::pun_write<std::uint32_t>(base_head + sizeof(std::uint32_t), 0);
        base_head += sizeof(std::uint32_t) * 2;
        std::unordered_map<std::pair<std::string, std::uint32_t>, std::uint32_t, utils::container_hasher<std::pair>> instance_indexes;
        for (auto ivar = cls.instance_variables.begin(); ivar != cls.instance_variables.end(); ++ivar)
        {
            if (auto cidx = class_indexes.find(ivar->host_name); cidx != class_indexes.end())
            {
                instance_indexes[{ivar->name, cidx->second}] = ivar - cls.instance_variables.begin();
                utils::pun_write(base_head, cidx->second);
            }
            else
            {
                error_builder << "Unable to find instance variable import " << ivar->host_name << "!";
                errors.push_back(error_builder.str());
                logger.debug(error_builder.str());
                error_builder.clear();
                continue;
            }
            logger.builder(logging::level::debug) << "Instance name: " << ivar->name << logging::logbuilder::end;
            logger.builder(logging::level::debug) << "Instance host: " << ivar->host_name << logging::logbuilder::end;
            logger.builder(logging::level::debug) << "Base head: " << static_cast<std::uintptr_t>(base_head - maybe_cls->mmapped_file) << logging::logbuilder::end;
            logger.builder(logging::level::debug) << "Current string offset: " << current_string_offset << logging::logbuilder::end;
            utils::pun_write<std::uint32_t>(base_head + sizeof(std::uint32_t), 0);
            utils::pun_write(base_head + sizeof(std::uint32_t) * 2, current_string_offset);
            base_head += sizeof(std::uint32_t) * 2 + sizeof(current_string_offset);
            utils::pun_write(maybe_cls->mmapped_file + current_string_offset, static_cast<std::uint32_t>(ivar->name.size()));
            std::memcpy(maybe_cls->mmapped_file + sizeof(std::uint32_t) + current_string_offset, ivar->name.c_str(), ivar->name.size());
            current_string_offset += ::round_off(ivar->name.size() + sizeof(std::uint32_t));
        }
        utils::pun_write(base_head, string_offset - bytecode_offset);
        base_head += sizeof(std::uint64_t);
        for (auto &method : compiled_methods)
        {
            logger.builder(logging::level::debug) << "Writing " << method.name << " size " << method.size << logging::logbuilder::end;
            logger.builder(logging::level::debug) << "Base head offset " << base_head - maybe_cls->mmapped_file << logging::logbuilder::end;
            for (auto &thunk : method.thunks)
            {
                switch (thunk.type)
                {
                case oops_bcode_compiler::compiler::thunk_type::CLASS:
                {
                    auto idx = class_indexes.find(thunk.name);
                    if (idx == class_indexes.end())
                    {
                        error_builder << "Unable to find class name " << thunk.name << " for compiled instruction " << thunk.instruction_idx << " in method " << method.name;
                        errors.push_back(error_builder.str());
                        logger.debug(error_builder.str());
                        error_builder.clear();
                        continue;
                    }
                    method.instructions[thunk.instruction_idx] = dethunk(thunk, method.instructions[thunk.instruction_idx], idx->second);
                    break;
                }
                case oops_bcode_compiler::compiler::thunk_type::METHOD:
                {
                    auto cidx = class_indexes.find(thunk.class_name);
                    if (cidx == class_indexes.end())
                    {
                        error_builder << "Unable to find class name " << thunk.name << " for compiled instruction " << thunk.instruction_idx << " in method " << method.name;
                        errors.push_back(error_builder.str());
                        logger.debug(error_builder.str());
                        error_builder.clear();
                        continue;
                    }
                    auto idx = method_indexes.find({thunk.name, cidx->second});
                    if (idx == method_indexes.end())
                    {
                        error_builder << "Unable to find method name " << thunk.name << " for compiled instruction " << thunk.instruction_idx << " in method " << method.name;
                        errors.push_back(error_builder.str());
                        logger.debug(error_builder.str());
                        error_builder.clear();
                        continue;
                    }
                    method.instructions[thunk.instruction_idx] = dethunk(thunk, method.instructions[thunk.instruction_idx], idx->second);
                    break;
                }
                case oops_bcode_compiler::compiler::thunk_type::IVAR:
                {
                    auto cidx = class_indexes.find(thunk.class_name);
                    if (cidx == class_indexes.end())
                    {
                        error_builder << "Unable to find class name " << thunk.name << " for compiled instruction " << thunk.instruction_idx << " in method " << method.name;
                        errors.push_back(error_builder.str());
                        logger.debug(error_builder.str());
                        error_builder.clear();
                        continue;
                    }
                    auto idx = instance_indexes.find({thunk.name, cidx->second});
                    if (idx == instance_indexes.end())
                    {
                        error_builder << "Unable to find instance variable name " << thunk.name << " for compiled instruction " << thunk.instruction_idx << " in method " << method.name;
                        errors.push_back(error_builder.str());
                        logger.debug(error_builder.str());
                        error_builder.clear();
                        continue;
                    }
                    method.instructions[thunk.instruction_idx] = dethunk(thunk, method.instructions[thunk.instruction_idx], idx->second);
                    break;
                }
                case oops_bcode_compiler::compiler::thunk_type::SVAR:
                {
                    auto cidx = class_indexes.find(thunk.class_name);
                    if (cidx == class_indexes.end())
                    {
                        error_builder << "Unable to find class name " << thunk.name << " for compiled instruction " << thunk.instruction_idx << " in method " << method.name;
                        errors.push_back(error_builder.str());
                        logger.debug(error_builder.str());
                        error_builder.clear();
                        continue;
                    }
                    auto idx = method_indexes.find({thunk.name, cidx->second});
                    if (idx == method_indexes.end())
                    {
                        error_builder << "Unable to find static variable name " << thunk.name << " for compiled instruction " << thunk.instruction_idx << " in method " << method.name;
                        errors.push_back(error_builder.str());
                        logger.debug(error_builder.str());
                        error_builder.clear();
                        continue;
                    }
                    method.instructions[thunk.instruction_idx] = dethunk(thunk, method.instructions[thunk.instruction_idx], idx->second);
                    break;
                }
                }
            }
            logger.builder(logging::level::debug) << "Dethunking complete" << logging::logbuilder::end;
            utils::pun_write<std::uint64_t>(base_head, method.size);
            base_head += sizeof(std::uint64_t);
            logger.builder(logging::level::debug) << "Method size complete" << logging::logbuilder::end;
            logger.builder(logging::level::debug) << "Base head offset " << base_head - maybe_cls->mmapped_file << logging::logbuilder::end;
            utils::pun_write<std::uint16_t>(base_head, method.instructions.size());
            utils::pun_write<std::uint16_t>(base_head + sizeof(std::uint16_t), method.stack_size);
            utils::pun_write<std::uint16_t>(base_head + sizeof(std::uint16_t) * 2, method.return_type | method.method_type << 4);
            utils::pun_write<std::uint16_t>(base_head + sizeof(std::uint16_t) * 3, method.arg_types.size());
            base_head += sizeof(std::uint16_t) * 4;
            logger.builder(logging::level::debug) << "Method meta complete" << logging::logbuilder::end;
            logger.builder(logging::level::debug) << "Base head offset " << base_head - maybe_cls->mmapped_file << logging::logbuilder::end;
            std::uint64_t arg_builder = 0;
            logger.builder(logging::level::debug) << "Arg types count " << method.arg_types.size() << logging::logbuilder::end;
            for (std::size_t i = 0; i < method.arg_types.size(); i++)
            {
                auto mod = i % (sizeof(std::uint64_t) / 4 * CHAR_BIT);
                logger.builder(logging::level::debug) << "mod " << mod << logging::logbuilder::end;
                logger.builder(logging::level::debug) << "Base head offset " << base_head - maybe_cls->mmapped_file << logging::logbuilder::end;
                arg_builder |= static_cast<std::uint64_t>(method.arg_types[i]) << (mod * 4);
                if (mod == sizeof(std::uint64_t) / 4 * CHAR_BIT - 1)
                {
                    utils::pun_write(base_head, arg_builder);
                    base_head += sizeof(arg_builder);
                    arg_builder = 0;
                }
                logger.builder(logging::level::debug) << "i " << i << logging::logbuilder::end;
                logger.builder(logging::level::debug) << "Base head offset " << base_head - maybe_cls->mmapped_file << logging::logbuilder::end;
            }
            if (method.arg_types.size() % (sizeof(std::uint64_t) / 4 * CHAR_BIT))
            {
                utils::pun_write(base_head, arg_builder);
                base_head += sizeof(arg_builder);
                arg_builder = 0;
            }
            logger.builder(logging::level::debug) << "Argument building complete" << logging::logbuilder::end;
            logger.builder(logging::level::debug) << "Base head offset " << base_head - maybe_cls->mmapped_file << logging::logbuilder::end;
            for (auto instruction : method.instructions)
            {
                utils::pun_write(base_head, instruction);
                base_head += sizeof(instruction);
            }
            logger.builder(logging::level::debug) << "Instruction copying complete" << logging::logbuilder::end;
            logger.builder(logging::level::debug) << "Base head offset " << base_head - maybe_cls->mmapped_file << logging::logbuilder::end;
            utils::pun_write<std::uintptr_t>(base_head, 0);
            base_head += sizeof(std::uintptr_t);
            std::uint64_t handle_builder = method.handle_map.size();
            for (std::size_t i = 0; i < method.handle_map.size(); i++)
            {
                auto mod = (i + 1) % (sizeof(std::uint64_t) / sizeof(std::uint16_t));
                handle_builder |= static_cast<std::uint64_t>(method.handle_map[i]) << (mod * sizeof(std::uint16_t) * CHAR_BIT);
                if (mod == sizeof(std::uint64_t) / sizeof(std::uint16_t) - 1)
                {
                    utils::pun_write(base_head, handle_builder);
                    base_head += sizeof(handle_builder);
                    handle_builder = 0;
                }
            }
            if ((method.handle_map.size() + 1) % (sizeof(std::uint64_t) / sizeof(std::uint16_t)))
            {
                utils::pun_write(base_head, handle_builder);
                base_head += sizeof(handle_builder);
            }
            logger.builder(logging::level::debug) << "Final base_head offset: " << base_head - maybe_cls->mmapped_file << logging::logbuilder::end;
        }
        platform::close_file_mapping(*maybe_cls, true);
        return errors;
    }
    return {"Unable to open file mapping!"};
}