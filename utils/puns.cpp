#include "puns.h"

#include <cstring>

using namespace oops_bcode_compiler::utils;

template <typename primitive>
std::enable_if_t<std::is_trivially_copyable_v<primitive> and std::is_default_constructible_v<primitive>, primitive> oops_bcode_compiler::utils::pun_read(void *from)
{
    primitive ret;
    std::memcpy(&ret, from, sizeof(primitive));
    return ret;
}

template <typename primitive>
std::enable_if_t<std::is_trivially_copyable_v<primitive> and std::is_default_constructible_v<primitive>, void> oops_bcode_compiler::utils::pun_write(void *to, primitive p)
{
    std::memcpy(to, &p, sizeof(primitive));
}

template <typename from_t, typename to_t>
std::enable_if_t<sizeof(from_t) == sizeof(to_t), to_t> oops_bcode_compiler::utils::pun_reinterpret(from_t from)
{
    return utils::pun_read<to_t>(&from);
}