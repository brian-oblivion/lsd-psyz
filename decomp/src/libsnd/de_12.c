#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/de_12", _SsSetNrpnVabAttr12);
#else
// NRPN data entry 12: the sustain's direction, up below 64, down from 64.
void _SsSetNrpnVabAttr12(short vabId, short prog, short tone, VagAtr vag,
                         short attr, unsigned char data) {
    struct Unk adsr;

    SsUtGetVagAtr(vabId, prog, tone, &vag);
    _SsUtResolveADSR(vag.adsr1, vag.adsr2, &adsr);
    if (data != 0 && data < 0x40) {
        adsr.unk10 = 0;
    } else if (data >= 0x40 && data < 0x80) {
        adsr.unk10 = 1;
    }
    _SsUtBuildADSR(&adsr, &vag.adsr1, &vag.adsr2);
    SsUtSetVagAtr(vabId, prog, tone, &vag);
}
#endif
