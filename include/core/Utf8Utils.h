#pragma once

#include <string>
#include <utfcpp/utf8.h>

namespace utf8_utils {
    inline bool is_all_chinese(const std::string &word) {
        auto it = utf8::iterator{word.begin(), word.begin(), word.end()};
        auto end = utf8::iterator{word.end(), word.begin(), word.end()};
        for (; it != end; ++it)
            if (*it < 0x4E00 || *it > 0x9FFF) return false;
        return true;
    }
}
