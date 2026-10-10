#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/vm_vol", _SsVmSetVol);
#else
// Sets the volume of every voice the sequence plays on the VAB's program
// from vol (the voice's level out of 127) and pan, through the VAB, program
// and tone volumes, the sequence's volume and the tone and program pans.
// Returns how many voices. Written from libsnd 3.3's SpuVmSetVol, which
// reads the tone's volume and pan at the voice's tone index alone, not
// offset by its program: kept as it is.
int _SsVmSetVol(
    short seq_sep_no, short vabId, short prog, short vol, short pan) {
    struct SeqStruct* score =
        &_ss_score[seq_sep_no & 0xFF][((u16)seq_sep_no >> 8) & 0xFF];
    int count = 0;
    int i;

    _SsVmVSetUp(vabId, prog);
    _svm_cur.seq_sep_no = seq_sep_no;
    for (i = 0; i < _SsVmMaxVoice; i++) {
        struct SpuVoice* v = &_svm_voice[i];
        unsigned level, l, r;
        unsigned p;

        if (v->seq_sep_no != seq_sep_no || v->prog != prog ||
            v->vabId != vabId) {
            continue;
        }
        level = v->voll1 * (u16)vol / 127;
        level = _svm_vh->mvol * (level * 0x3FFF) / 16129;
        level = level * _svm_pg[v->prog].mvol;
        level = level * _svm_tn[v->tone].vol / 16129;
        l = level * (u16)score->voll / 127;
        r = level * (u16)score->volr / 127;

        p = _svm_tn[v->tone].pan;
        if (p < 0x40) {
            r = r * p / 63;
        } else {
            l = l * (0x7F - p) / 63;
        }
        p = _svm_pg[v->fake_program].mpan;
        if (p < 0x40) {
            r = r * p / 63;
        } else {
            l = l * (0x7F - p) / 63;
        }
        p = (u8)pan;
        if (p < 0x40) {
            r = r * p / 63;
        } else {
            l = l * (0x7F - p) / 63;
        }
        if (_svm_stereo_mono == 1) {
            if (l < r) {
                l = r;
            } else {
                r = l;
            }
        }
        _svm_sreg_buf[i].volume.left = l * l / 16383;
        _svm_sreg_buf[i].volume.right = r * r / 16383;
        _svm_sreg_dirty[i] |= 3;
        count++;
    }
    return count;
}
#endif
