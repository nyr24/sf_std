#pragma once

#include "defines.hpp"
#include "traits.hpp"

namespace sf {

struct GeneralPurposeAllocator { 
    void* allocate(u32 size, u16 alignment) noexcept;
    usize allocate_handle(u32 size, u16 alignment) noexcept;
    void* handle_to_ptr(usize handle) const noexcept;
    usize ptr_to_handle(void* ptr) const noexcept;
    ReallocReturn reallocate(void* addr, u32 new_size, u16 alignment) noexcept;
    ReallocReturnHandle reallocate_handle(usize handle, u32 new_size, u16 alignment) noexcept;
    void free(void* addr) noexcept;
    void free_handle(usize handle) noexcept;
    void clear() noexcept {}
};

GeneralPurposeAllocator& get_current_gpa();

} // sf
