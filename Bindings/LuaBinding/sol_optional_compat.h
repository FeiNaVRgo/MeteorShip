#ifndef METEORSHIP_SOL_OPTIONAL_COMPAT_H
#define METEORSHIP_SOL_OPTIONAL_COMPAT_H

#include <sol/sol.hpp>

#include <optional>
#include <type_traits>

namespace sol {
    template<typename T>
    struct is_container<std::optional<T>> : std::false_type {};
}

#endif
