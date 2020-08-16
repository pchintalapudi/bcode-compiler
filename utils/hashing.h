#ifndef UTILS_HASHING
#define UTILS_HASHING

#include <cstddef>
#include <type_traits>

namespace oops_bcode_compiler
{
    namespace utils
    {

        template<template<typename... Args> class Container>
        struct container_hasher {
            template<typename... Args>
            std::size_t operator()(const Container<Args...>& to_hash) const {
                return 0;
            }
        };
    } // namespace utils
} // namespace oops_bcode_compiler

#endif /* UTILS_HASHING */
