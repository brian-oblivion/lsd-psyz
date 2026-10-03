#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/de_0", _SsSetNrpnVabAttr0);
#else
// NRPN data entry 0: the tone's priority.
void _SsSetNrpnVabAttr0(short vabId, short prog, short tone, VagAtr vag,
                        short attr, unsigned char data) {
    SsUtGetVagAtr(vabId, prog, tone, &vag);
    vag.prior = data;
    SsUtSetVagAtr(vabId, prog, tone, &vag);
}
#endif
