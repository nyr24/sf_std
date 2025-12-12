#pragma once

#include "defines.hpp"

namespace sf {

void*   platform_mem_alloc(u64 byte_size, u16 alignment);
u32     platform_get_mem_page_size();
void    platform_console_write(char* message_buff, u16 written_count, u8 color);
void    platform_console_write_error(char* message_buff, u16 written_count, u8 color);
f64     platform_get_abs_time();
void    platform_sleep(u64 ms);

} // sf
