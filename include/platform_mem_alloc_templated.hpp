#pragma once

#include "defines.hpp"
#include "platform.hpp"

namespace sf {

template<typename T, bool should_align>
T* platform_mem_alloc_typed(u64 count) {
    if constexpr (should_align) {
        return static_cast<T*>(platform_mem_alloc(sizeof(T) * count, alignof(T)));
    } else {
        return static_cast<T*>(platform_mem_alloc(sizeof(T) * count, 0));
    }
}

} // sf
