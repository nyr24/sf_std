#include "general_purpose_allocator.hpp"
#include "traits.hpp"
#include "asserts_sf.hpp"
#include "constants.hpp"
#include "defines.hpp"
#include "memory_sf.hpp"

namespace sf {

static GeneralPurposeAllocator gpa_instance{};

GeneralPurposeAllocator& get_current_gpa() {
    return gpa_instance;
}

void* GeneralPurposeAllocator::allocate(u32 size, u16 alignment) noexcept {
    return sf_mem_alloc(size, alignment);
}

ReallocReturn GeneralPurposeAllocator::reallocate(void* addr, u32 new_size, u16 alignment) noexcept {
    return {sf_mem_realloc(addr, new_size), false};
}

void GeneralPurposeAllocator::free(void* addr) noexcept {
    sf_mem_free(addr);
    addr = nullptr;
}

void GeneralPurposeAllocator::free_handle(usize handle) noexcept
{
    SF_ASSERT_MSG(false, "You are using general purpose allocator with handles");
}

void* GeneralPurposeAllocator::handle_to_ptr(usize handle) const noexcept {
    SF_ASSERT_MSG(false, "You are using general purpose allocator with handles");
    return nullptr;
}

usize GeneralPurposeAllocator::ptr_to_handle(void* ptr) const noexcept {
    SF_ASSERT_MSG(false, "You are using general purpose allocator with handles");
    return INVALID_ALLOC_HANDLE;
}

} // sf
