#include "libsnd_private.h"

short SsUtGetVabHdr(short vabId, VabHdr* vabhdrptr) {
    if (_SsVmVSetUp(vabId, 0) != 0) {
        return -1;
    }
    *vabhdrptr = *_svm_vh;
    return 0;
}
