#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/de_11", _SsSetNrpnVabAttr11);
#else
// NRPN data entry 11: an exponential release at the rate.
void _SsSetNrpnVabAttr11(short vabId, short prog, short tone, VagAtr vag,
                         short attr, unsigned char data) {
    struct Unk adsr;

    SsUtGetVagAtr(vabId, prog, tone, &vag);
    _SsUtResolveADSR(vag.adsr1, vag.adsr2, &adsr);
    adsr.unkE = 1;
    adsr.unk8 = data;
    _SsUtBuildADSR(&adsr, &vag.adsr1, &vag.adsr2);
    SsUtSetVagAtr(vabId, prog, tone, &vag);
}
#endif
