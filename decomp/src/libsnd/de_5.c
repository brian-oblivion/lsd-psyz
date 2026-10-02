#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/de_5", _SsSetNrpnVabAttr5);
#else
// NRPN data entry 5: an exponential attack at the rate.
void _SsSetNrpnVabAttr5(short vabId, short prog, short tone, VagAtr vag,
                        short attr, unsigned char data) {
    struct Unk adsr;

    SsUtGetVagAtr(vabId, prog, tone, &vag);
    _SsUtResolveADSR(vag.adsr1, vag.adsr2, &adsr);
    adsr.unkA = 1;
    adsr.unk0 = data;
    _SsUtBuildADSR(&adsr, &vag.adsr1, &vag.adsr2);
    SsUtSetVagAtr(vabId, prog, tone, &vag);
}
#endif
