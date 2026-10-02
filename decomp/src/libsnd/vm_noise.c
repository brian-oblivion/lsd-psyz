#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/vm_noise", _SsVmNoiseOnWithAdsr);

INCLUDE_ASM("asm/nonmatchings/libsnd/vm_noise", _SsVmNoiseOff);

INCLUDE_ASM("asm/nonmatchings/libsnd/vm_noise", _SsVmNoiseOn);
#else
// Written from libsnd 3.3's SpuVmNoiseOnWithAdsr, SpuVmNoiseOff and
// SpuVmNoiseOn.

void _SsVmNoiseOnWithAdsr(
    short voll, short volr, unsigned short adsr1, unsigned short adsr2) {
    int voice;

    _svm_cur.tone_prior = 0x7F;
    voice = (u8)_SsVmAlloc(0xFF);
    _svm_cur.voice = voice;
    if (voice < _SsVmMaxVoice) {
        vmNoiseOn2(voice, voll, volr, adsr1, adsr2);
    }
}

void _SsVmNoiseOff(void) {
    int i;

    for (i = 0; i < _SsVmMaxVoice; i++) {
        if (_svm_voice[i].unk1b == 2) {
            vmNoiseOff(i);
        }
    }
}

void _SsVmNoiseOn(short voll, short volr) {
    _SsVmNoiseOnWithAdsr(voll, volr, 0x80FF, 0x5FC8);
}
#endif
