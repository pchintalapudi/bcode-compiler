#ifndef UTILS_PUNS
#define UTILS_PUNS
#include <type_traits>

namespace oops_bcode_compiler {
    namespace utils {
        template<typename primitive>
        std::enable_if_t<std::is_trivially_copyable_v<primitive>, primitive> pun_read(void* from);
        template<typename primitive>
        std::enable_if_t<std::is_trivially_copyable_v<primitive>, void> pun_write(void* to, primitive p);
        template<typename from_t, typename to_t>
        std::enable_if_t<sizeof(from_t) == sizeof(to_t), to_t> pun_reinterpret(from_t from);
    }
}
#endif /* UTILS_PUNS */
