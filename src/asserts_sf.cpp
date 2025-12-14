#include "asserts_sf.hpp"
#include "logger.hpp"

namespace sf {

void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line) {
    sf::log_output(LogLevel::LOG_LEVEL_FATAL, "Assertion failure: {},\n\tmessage: {},\n\tin file: {},\n\tline: {}\n", expression, message, file, line);
}

} // sf

