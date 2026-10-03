#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/de_16", _SsSetNrpnVabAttr16);
#else
// NRPN data entry 16: the reverb depth.
void _SsSetNrpnVabAttr16(short vabId, short prog, short tone, VagAtr vag,
                         short attr, unsigned char data) {
    SsUtSetReverbDepth(data, data);
}
#endif
