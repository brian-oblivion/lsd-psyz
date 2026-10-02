#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/vm_no2", vmNoiseOn2);
#else
// Keys the voice on the noise generator at the given volumes; it becomes the
// only noise voice. The ADSR words are not read. Written from libsnd 3.3's
// vmNoiseOn2.
void vmNoiseOn2(u8 voice, u16 voll, u16 volr, u16 adsr1, u16 adsr2) {
    u16 bitsLo, bitsHi;
    int i;

    _svm_sreg_buf[voice].volume.right = volr;
    _svm_sreg_buf[voice].volume.left = voll;
    _svm_sreg_dirty[voice] |= 3;
    if (voice < 16) {
        bitsLo = 1 << voice;
        bitsHi = 0;
    } else {
        bitsLo = 0;
        bitsHi = 1 << (voice - 16);
    }
    _svm_voice[voice].unk04 = 10;
    for (i = 0; i < _SsVmMaxVoice; i++) {
        _svm_voice[i].unk1b &= 1;
    }
    _svm_voice[voice].unk1b = 2;
    _svm_voice[voice].unk2 = 0;
    _svm_okon1 |= bitsLo;
    _svm_okon2 |= bitsHi;
    _svm_okof1 &= ~_svm_okon1;
    _svm_okof2 &= ~_svm_okon2;
    SPUW(noise_mode[0], bitsLo);
    SPUW(noise_mode[1], bitsHi);
}
#endif
