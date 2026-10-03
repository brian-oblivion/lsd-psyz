#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/de_9", _SsSetNrpnVabAttr9);
#else
// NRPN data entry 9: an exponential sustain at the rate.
void _SsSetNrpnVabAttr9(short vabId, short prog, short tone, VagAtr vag,
                        short attr, unsigned char data) {
    struct Unk adsr;

    SsUtGetVagAtr(vabId, prog, tone, &vag);
    _SsUtResolveADSR(vag.adsr1, vag.adsr2, &adsr);
    adsr.unkC = 1;
    adsr.unk6 = data;
    _SsUtBuildADSR(&adsr, &vag.adsr1, &vag.adsr2);
    SsUtSetVagAtr(vabId, prog, tone, &vag);
}
#endif
