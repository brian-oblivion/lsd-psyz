#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/ut_gvh", SsUtGetVabHdr);
#else
short SsUtGetVabHdr(short vabId, VabHdr* vabhdrptr) {
    if (_SsVmVSetUp(vabId, 0) != 0) {
        return -1;
    }
    *vabhdrptr = *_svm_vh;
    return 0;
}
#endif
