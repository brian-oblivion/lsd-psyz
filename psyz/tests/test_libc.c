#include <psyz.h>
#include <rand.h>
#include <libc.h>
#include <string.h>
#include "ztest.h"

// libc2's rand() starts from a zero seed and returns bits 16 to 30 of its
// linear congruential state.
ZTEST(libc, rand_sequence) {
    srand(0);
    zexpect_s32_eq(0, rand());
    zexpect_s32_eq(21468, rand());
    zexpect_s32_eq(9988, rand());
    srand(1);
    zexpect_s32_eq(16838, rand());
    zexpect_s32_eq(5758, rand());
    zexpect_s32_eq(10113, rand());
}

ZTEST(libc, rand_max) {
    zexpect_s32_eq(0x7FFF, RAND_MAX);
    srand(12345);
    for (int i = 0; i < 1000; i++) {
        int r = rand();
        zexpect_s32_ge(0, r);
        zexpect_s32_le(RAND_MAX, r);
    }
}

ZTEST(libc, itoa_decimal) {
    zexpect_s32_eq(0, strcmp("0", itoa(0)));
    zexpect_s32_eq(0, strcmp("1234", itoa(1234)));
    zexpect_s32_eq(0, strcmp("-2147483648", itoa(-2147483647 - 1)));
}
