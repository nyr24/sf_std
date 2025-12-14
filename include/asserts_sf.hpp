#pragma once

#include "defines.hpp"

#ifdef SF_DEBUG

#if _MSC_VER
#include <intrin.h>
#define DEBUG_BREAK() __debugbreak()
#else
#define DEBUG_BREAK() __builtin_trap()
#endif

namespace sf {

void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line);

} // sf

#define SF_ASSERT(expr)                                                         \
    {                                                                           \
        if (expr) {                                                             \
        } else {                                                                \
            sf::report_assertion_failure(#expr, "", __FILE__, __LINE__);        \
            DEBUG_BREAK();                                                      \
        }                                                                       \
    }                                                                           \

#define SF_ASSERT_MSG(expr, msg)                                                \
    {                                                                           \
        if (expr) {                                                             \
        } else {                                                                \
            sf::report_assertion_failure(#expr, msg, __FILE__, __LINE__);       \
            DEBUG_BREAK();                                                      \
        }                                                                       \
    }                                                                           \

#else

#define SF_ASSERT(expr)
#define SF_ASSERT_MSG(expr, msg)

#endif // SF_DEBUG
