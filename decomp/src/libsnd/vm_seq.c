#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/vm_seq", _SsVmSetSeqVol);

INCLUDE_ASM("asm/nonmatchings/libsnd/vm_seq", _SsVmGetSeqVol);

INCLUDE_ASM("asm/nonmatchings/libsnd/vm_seq", _SsVmGetSeqLVol);

INCLUDE_ASM("asm/nonmatchings/libsnd/vm_seq", _SsVmGetSeqRVol);

INCLUDE_ASM("asm/nonmatchings/libsnd/vm_seq", _SsVmSeqKeyOff);
#else
// Written from libsnd 3.3's SpuVmSetSeqVol, SpuVmGetSeqVol, SpuVmGetSeqLVol,
// SpuVmGetSeqRVol and SpuVmSeqKeyOff. A sequence is named by its access
// number in the low byte and its sequence number in the high byte.

static struct SeqStruct* Score(short seq_sep_no) {
    return &_ss_score[seq_sep_no & 0xFF][((u16)seq_sep_no >> 8) & 0xFF];
}

// Sets the sequence's volume (each side at most 127), which the key-ons that
// follow use. Mode 1 also sets the voices the sequence is playing to it.
void _SsVmSetSeqVol(
    short seq_sep_no, unsigned short voll, unsigned short volr, short mode) {
    struct SeqStruct* score = Score(seq_sep_no);
    int i;

    _svm_cur.seq_sep_no = seq_sep_no;
    score->voll = voll;
    score->volr = volr;
    if ((u16)score->voll >= 0x80) {
        score->voll = 0x7F;
    }
    if ((u16)score->volr >= 0x80) {
        score->volr = 0x7F;
    }
    if (mode != 1) {
        return;
    }
    for (i = 0; i < _SsVmMaxVoice; i++) {
        if (_svm_voice[i].unke == seq_sep_no) {
            _svm_sreg_buf[i].volume.left = voll * 129;
            _svm_sreg_buf[i].volume.right = volr * 129;
            _svm_sreg_dirty[i] |= 3;
        }
    }
}

void _SsVmGetSeqVol(short seq_sep_no, short* voll, short* volr) {
    struct SeqStruct* score = Score(seq_sep_no);

    _svm_cur.seq_sep_no = seq_sep_no;
    *voll = score->voll;
    *volr = score->volr;
}

short _SsVmGetSeqLVol(short seq_sep_no) {
    _svm_cur.seq_sep_no = seq_sep_no & 0xFF;
    return Score(seq_sep_no)->voll;
}

short _SsVmGetSeqRVol(short seq_sep_no) {
    _svm_cur.seq_sep_no = seq_sep_no;
    return Score(seq_sep_no)->volr;
}

// Keys off every voice the sequence is playing.
void _SsVmSeqKeyOff(s16 seq_sep_num) {
    int i;

    for (i = 0; i < _SsVmMaxVoice; i++) {
        if (_svm_voice[i].unke == seq_sep_num) {
            _svm_cur.voice = i;
            _SsVmKeyOffNow(0);
        }
    }
}
#endif
