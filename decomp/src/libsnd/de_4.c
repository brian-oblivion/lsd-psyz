#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/de_4", _SsSetNrpnVabAttr4);
#else
// NRPN data entry 4: a linear attack at the rate.
void _SsSetNrpnVabAttr4(short vabId, short prog, short tone, VagAtr vag,
                        short attr, unsigned char data) {
    struct Unk adsr;

    SsUtGetVagAtr(vabId, prog, tone, &vag);
    _SsUtResolveADSR(vag.adsr1, vag.adsr2, &adsr);
    adsr.unkA = 0;
    adsr.unk0 = data;
    _SsUtBuildADSR(&adsr, &vag.adsr1, &vag.adsr2);
    SsUtSetVagAtr(vabId, prog, tone, &vag);
}
#endif
