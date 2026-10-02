#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/vm_no1", vmNoiseOn);
#else
// Keys the voice on the noise generator for _svm_cur's tone, at the
// sequence's volume, with the noise clock set from the note. Written from
// libsnd 3.3's vmNoiseOn.
void vmNoiseOn(char voice_) {
    u8 voice = voice_;
    unsigned seqL = 127;
    unsigned seqR = 127;
    unsigned l, r;
    unsigned pan;
    u16 bitsLo, bitsHi;
    u16 cnt;
    int i;

    // Retail reads the score for sound effects (0x21) too, past the end of
    // _ss_score; take them at full volume.
    if (_svm_cur.seq_sep_no != 0x21) {
        struct SeqStruct* score =
            &_ss_score[_svm_cur.seq_sep_no & 0xFF]
                      [((u16)_svm_cur.seq_sep_no >> 8) & 0xFF];
        seqL = (u16)score->voll;
        seqR = (u16)score->volr;
    }
    l = seqL * 129 * (u8)_svm_cur.mvol / 127 * (u8)_svm_cur.tone_vol / 127;
    r = seqR * 129 * (u8)_svm_cur.mvol / 127 * (u8)_svm_cur.tone_vol / 127;

    pan = (u8)_svm_cur.tone_pan;
    if (pan < 64) {
        r = r * pan / 63;
    } else {
        l = l * (127 - pan) / 63;
    }
    pan = (u8)_svm_cur.mpan;
    if (pan < 64) {
        r = r * pan / 63;
    } else {
        l = l * (127 - pan) / 63;
    }
    pan = (u8)_svm_cur.pan;
    if (pan < 64) {
        r = r * pan / 63;
    } else {
        l = l * (127 - pan) / 63;
    }
    if (_svm_stereo_mono == 1) {
        if (l < r) {
            l = r;
        } else {
            r = l;
        }
    }

    cnt = SPUR(spucnt);
    cnt = (cnt & 0xC0FF) |
          ((((u8)_svm_cur.note - (u8)_svm_cur.tone_center) & 0x3F) << 8);
    SPUW(spucnt, cnt);

    _svm_sreg_buf[voice].volume.right = r;
    _svm_sreg_buf[voice].volume.left = l;
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
    _svm_okon1 |= bitsLo;
    _svm_okon2 |= bitsHi;
    _svm_okof1 &= ~_svm_okon1;
    _svm_okof2 &= ~_svm_okon2;
    if (_svm_cur.tone_mode & 4) {
        _svm_orev1 |= bitsLo;
        _svm_orev2 |= bitsHi;
    } else {
        _svm_orev1 &= ~bitsLo;
        _svm_orev2 &= ~bitsHi;
    }
    SPUW(noise_mode[0], bitsLo);
    SPUW(noise_mode[1], bitsHi);
}
#endif
