#include "general_purpose_allocator.hpp"
#include "traits.hpp"
#include "asserts_sf.hpp"
#include "constants.hpp"
#include "defines.hpp"
#include "memory_sf.hpp"

namespace sf {

static GeneralPurposeAllocator gpa;

GeneralPurposeAllocator* get_current_gpa()
{
    return &gpa;
}

void* GeneralPurposeAllocator::allocate(u32 size, u16 alignment) noexcept {
    return sf_mem_alloc(size, alignment);
}

ReallocReturn GeneralPurposeAllocator::reallocate(void* addr, u32 new_size, u16 alignment) noexcept {
    return {sf_mem_realloc(addr, new_size), false};
}

void GeneralPurposeAllocator::free(void* addr, u16 alignment) noexcept {
    sf_mem_free(addr, alignment);
    addr = nullptr;
}

void* GeneralPurposeAllocator::handle_to_ptr(usize handle) const noexcept {
    SF_ASSERT_MSG(false, "You are using GeneralPurposeAllocator with handles");
    return nullptr;
}

usize GeneralPurposeAllocator::ptr_to_handle(void* ptr) const noexcept {
    SF_ASSERT_MSG(false, "You are using GeneralPurposeAllocator with handles");
    return INVALID_ALLOC_HANDLE;
}

usize GeneralPurposeAllocator::allocate_handle(u32 size, u16 alignment) noexcept {
    SF_ASSERT_MSG(false, "You are using GeneralPurposeAllocator with handles");
    return INVALID_ALLOC_HANDLE;
}

ReallocReturnHandle GeneralPurposeAllocator::reallocate_handle(usize handle, u32 new_size, u16 alignment) noexcept {
    SF_ASSERT_MSG(false, "You are using GeneralPurposeAllocator with handles");
    return {INVALID_ALLOC_HANDLE, false};
}

void GeneralPurposeAllocator::free_handle(usize handle, u16 align) noexcept {
    SF_ASSERT_MSG(false, "You are using GeneralPurposeAllocator with handles");
}

} // sf
