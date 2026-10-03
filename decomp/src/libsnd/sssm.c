#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/sssm", SsSetMute);
#else
// Written from libsnd 3.3's: SS_MUTE_ON mutes the SPU, SS_MUTE_OFF unmutes
// it.
void SsSetMute(char mode) {
    if (mode == SS_MUTE_OFF || mode == SS_MUTE_ON) {
        SpuSetMute(mode);
    }
}
#endif
