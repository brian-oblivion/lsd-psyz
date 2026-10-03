#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/de_13", _SsSetNrpnVabAttr13);
#else
// NRPN data entry 13: the vibrato time.
void _SsSetNrpnVabAttr13(short vabId, short prog, short tone, VagAtr vag,
                         short attr, unsigned char data) {
    struct Unk adsr;

    SsUtGetVagAtr(vabId, prog, tone, &vag);
    _SsUtResolveADSR(vag.adsr1, vag.adsr2, &adsr);
    vag.vibT = data;
    _SsUtBuildADSR(&adsr, &vag.adsr1, &vag.adsr2);
    SsUtSetVagAtr(vabId, prog, tone, &vag);
}
#endif
