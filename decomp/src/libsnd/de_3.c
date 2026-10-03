#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/de_3", _SsSetNrpnVabAttr3);
#else
// NRPN data entry 3: the tone's highest note.
void _SsSetNrpnVabAttr3(short vabId, short prog, short tone, VagAtr vag,
                        short attr, unsigned char data) {
    SsUtGetVagAtr(vabId, prog, tone, &vag);
    vag.max = data;
    SsUtSetVagAtr(vabId, prog, tone, &vag);
}
#endif
