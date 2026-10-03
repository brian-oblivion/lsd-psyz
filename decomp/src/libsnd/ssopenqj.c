#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/ssopenqj", SsSeqOpenJ);
#else
// Opens the SEQ at addr on the first free access number and returns it, or
// -1. SsFCALL's MIDI event table is the caller's. Written from libsnd 3.3's
// SsSeqOpen.
short SsSeqOpenJ(u_long* addr, short vab_id) {
    int flag;

    if (_snd_openflag == -1) {
        printf("Can't Open Sequence data any more\n\n");
        return -1;
    }
    for (flag = 0; _snd_openflag & (1 << flag); flag++) {
    }
    _snd_openflag |= 1 << flag;
    if (_SsInitSoundSeq(flag, vab_id, addr) == -1) {
        return -1;
    }
    return flag;
}
#endif
