#ifndef PARSE_RTC_ID_H
#define PARSE_RTC_ID_H

#include <charconv>
#include <string>
#include <system_error>

/* Digits-only rtc id suffix. Rejects empty, junk, and unsigned long overflow (stoul abort class). */
inline bool ParseRtcId(const std::string &s, unsigned long &out)
{
    if (s.empty()) {
        return false;
    }
    for (char c : s) {
        if (c < '0' || c > '9') {
            return false;
        }
    }
    unsigned long value = 0;
    const char *first = s.data();
    const char *last = first + s.size();
    auto result = std::from_chars(first, last, value);
    if (result.ec != std::errc() || result.ptr != last) {
        return false;
    }
    out = value;
    return true;
}

#endif
