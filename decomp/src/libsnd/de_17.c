#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/de_17", _SsSetNrpnVabAttr17);
#else
// NRPN data entry 17: the reverb feedback.
void _SsSetNrpnVabAttr17(short vabId, short prog, short tone, VagAtr vag,
                         short attr, unsigned char data) {
    SsUtSetReverbFeedback(data);
}
#endif
