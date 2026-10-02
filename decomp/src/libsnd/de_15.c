#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/de_15", _SsSetNrpnVabAttr15);
#else
// NRPN data entry 15: the reverb type.
void _SsSetNrpnVabAttr15(short vabId, short prog, short tone, VagAtr vag,
                         short attr, unsigned char data) {
    SsUtSetReverbType(data);
}
#endif
