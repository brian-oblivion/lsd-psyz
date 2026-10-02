#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/de_2", _SsSetNrpnVabAttr2);
#else
// NRPN data entry 2: the tone's lowest note.
void _SsSetNrpnVabAttr2(short vabId, short prog, short tone, VagAtr vag,
                        short attr, unsigned char data) {
    SsUtGetVagAtr(vabId, prog, tone, &vag);
    vag.min = data;
    SsUtSetVagAtr(vabId, prog, tone, &vag);
}
#endif
