#include "../parse_rtc_id.h"
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <string>

static void ExpectOk(const char *s, unsigned long want)
{
    unsigned long out = 999;
    assert(ParseRtcId(s, out));
    assert(out == want);
}

static void ExpectFail(const char *s)
{
    unsigned long out = 42;
    assert(!ParseRtcId(s, out));
}

static bool StoulThrows(const std::string &s)
{
    try {
        (void)std::stoul(s);
        return false;
    } catch (const std::out_of_range &) {
        return true;
    } catch (const std::invalid_argument &) {
        return true;
    }
}

int main()
{
    ExpectOk("0", 0);
    ExpectOk("1", 1);
    ExpectOk("42", 42);
    ExpectFail("");
    ExpectFail("-1");
    ExpectFail("12a");
    ExpectFail("a12");
    ExpectFail(" 1");
    ExpectFail("1 ");
    ExpectFail("rtc");

    /* leftover: get_wall_clock_rtc_id find("rtc") + stoul on empty / overflow */
    assert(StoulThrows(""));
    assert(StoulThrows("lock"));
    const std::string overflow(40, '9');
    assert(StoulThrows(overflow));
    ExpectFail(overflow.c_str());

    std::puts("parse_rtc_id_host_test OK");
    return 0;
}
