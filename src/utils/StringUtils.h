#pragma once

#include <string>

namespace string_utils {

// Returns true if value is non-empty and contains only ASCII digit characters.
inline bool isDigitsOnly(const std::string &value) {
    return !value.empty() &&
           value.find_first_not_of("0123456789") == std::string::npos;
}

} // namespace string_utils
