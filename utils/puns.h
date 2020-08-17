#ifndef UTILS_PUNS
#define UTILS_PUNS
#include <type_traits>
#include <cstring>

namespace oops_bcode_compiler
{
    namespace utils
    {
        template <typename primitive>
        std::enable_if_t<std::is_trivially_copyable_v<primitive> and std::is_default_constructible_v<primitive>, primitive> pun_read(void *from)
        {
            primitive ret;
            std::memcpy(&ret, from, sizeof(primitive));
            return ret;
        }

        template <typename primitive>
        std::enable_if_t<std::is_trivially_copyable_v<primitive> and std::is_default_constructible_v<primitive>, void> pun_write(void *to, primitive p)
        {
            std::memcpy(to, &p, sizeof(primitive));
        }

        template <typename to_t, typename from_t>
        std::enable_if_t<sizeof(from_t) == sizeof(to_t), to_t> pun_reinterpret(from_t from)
        {
            return utils::pun_read<to_t>(&from);
        }
    } // namespace utils
} // namespace oops_bcode_compiler
#endif /* UTILS_PUNS */
