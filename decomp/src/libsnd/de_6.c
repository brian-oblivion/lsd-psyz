#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/de_6", _SsSetNrpnVabAttr6);
#else
// NRPN data entry 6: the decay rate.
void _SsSetNrpnVabAttr6(short vabId, short prog, short tone, VagAtr vag,
                        short attr, unsigned char data) {
    struct Unk adsr;

    SsUtGetVagAtr(vabId, prog, tone, &vag);
    _SsUtResolveADSR(vag.adsr1, vag.adsr2, &adsr);
    adsr.unk2 = data;
    _SsUtBuildADSR(&adsr, &vag.adsr1, &vag.adsr2);
    SsUtSetVagAtr(vabId, prog, tone, &vag);
}
#endif
