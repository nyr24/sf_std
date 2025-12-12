#include "defines.hpp"

#if defined(SF_PLATFORM_WINDOWS)
#include "platform.hpp"
#include "utility.hpp"
#include "defines.hpp"
#include "logger.hpp"
#include "fixed_array.hpp"
#include <Windows.h>
#include <windowsx.h>

namespace sf {

void* platform_mem_alloc(u64 byte_size, u16 alignment = 0) {
    HANDLE heap_handle = GetProcessHeap();
    void* ptr = HeapAlloc(heap_handle, HEAP_ZERO_MEMORY, byte_size);
    if (!ptr) {
        panic("Out of memory");
    }
    return ptr;
}

f64 platform_get_abs_time() {
    LARGE_INTEGER now_time;
    QueryPerformanceCounter(&now_time);
    return (f64)now_time.QuadPart * clock_frequency;
}

static const FixedArray<u8, static_cast<u8>(LogLevel::COUNT)> levels = { 64, 4, 6, 2, 1, 8 };

void platform_console_write(char* message_buff, u16 written_count, u8 color) {
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(console_handle, levels[color]);

    OutputDebugStringA(message_buff);
    DWORD res_count_written;
    WriteConsoleA(console_handle, message_buff, (DWORD)written_count, &res_count_written, 0);
}

void platform_console_write_error(char* message_buff, u16 written_count, u8 color) {
    HANDLE console_handle = GetStdHandle(STD_ERROR_HANDLE);
    SetConsoleTextAttribute(console_handle, levels[color]);

    OutputDebugStringA(message_buff);
    DWORD res_count_written;
    WriteConsoleA(console_handle, message_buff, (DWORD)written_count, &res_count_written, 0);
}

void platform_sleep(u64 ms) {
    Sleep(ms);
}

u32 platform_get_mem_page_size() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return static_cast<u32>(si.dwPageSize);
}

} // sf

#endif // SF_PLATFORM_WINDOWS
