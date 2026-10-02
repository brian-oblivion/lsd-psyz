#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/ssmark", SsSetMarkCallback);
#else
// The callback NRPN 40 calls with its data (see _SsContNrpn1).
void SsSetMarkCallback(
    short access_num, short seq_num, SsMarkCallbackProc proc) {
    _SsMarkCallback[access_num][seq_num] = (SndSsMarkCallbackProc)proc;
}
#endif
