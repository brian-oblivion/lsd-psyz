#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/de_18", _SsSetNrpnVabAttr18);
#else
// NRPN data entry 18: the reverb delay.
void _SsSetNrpnVabAttr18(short vabId, short prog, short tone, VagAtr vag,
                         short attr, unsigned char data) {
    SsUtSetReverbDelay(data);
}
#endif
