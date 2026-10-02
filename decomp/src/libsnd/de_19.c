#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/de_19", _SsSetNrpnVabAttr19);
#else
// NRPN data entry 19: the reverb delay.
void _SsSetNrpnVabAttr19(short vabId, short prog, short tone, VagAtr vag,
                         short attr, unsigned char data) {
    SsUtSetReverbDelay(data);
}
#endif
