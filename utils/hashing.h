#ifndef UTILS_HASHING
#define UTILS_HASHING

#include <cstddef>
#include <type_traits>

namespace oops_bcode_compiler
{
    namespace utils
    {
        template <class T>
        inline std::size_t hash_combine(std::size_t seed, const T &v)
        {
            std::hash<T> hasher;
            seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }

        template <template <typename... Args> class Container>
        struct container_hasher
        {
            template <typename... Args>
            std::size_t operator()(const Container<Args...> &to_hash) const
            {
                if constexpr (sizeof...(Args) == 0)
                {
                    return 0;
                }
                else
                {
                    return this->operator()<0, Args...>(to_hash);
                }
            }
            template <std::size_t idx, typename... Args>
            std::size_t operator()(const Container<Args...> &to_hash) const
            {
                std::size_t single_hash = hash_combine(0, std::get<idx>(to_hash));
                if constexpr (idx < sizeof...(Args) - 1)
                {
                    return hash_combine(single_hash, this->operator()<idx + 1, Args...>(to_hash));
                }
                else
                {
                    return single_hash;
                }
            }
        };
    } // namespace utils
} // namespace oops_bcode_compiler

#endif /* UTILS_HASHING */
