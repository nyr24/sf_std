#include "utility.hpp"
#include "logger.hpp"

namespace sf {

[[noreturn]] void panic(const char* message) {
    LOG_FATAL("{}", message);
    std::exit(1);
}

[[noreturn]] void unreachable() {
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
    __assume(false);
#else // GCC, Clang
    __builtin_unreachable();
#endif
}

} // sf
