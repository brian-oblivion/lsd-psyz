#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/de_1", _SsSetNrpnVabAttr1);
#else
// NRPN data entry 1: the tone's mode; 4 turns the reverb on, 0 off.
void _SsSetNrpnVabAttr1(short vabId, short prog, short tone, VagAtr vag,
                        short attr, unsigned char data) {
    SsUtGetVagAtr(vabId, prog, tone, &vag);
    vag.mode = data;
    SsUtSetVagAtr(vabId, prog, tone, &vag);
    if (data == 0) {
        SsUtReverbOff();
    } else if (data == 4) {
        SsUtReverbOn();
    }
}
#endif
