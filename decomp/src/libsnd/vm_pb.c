#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/vm_pb", _SsVmPBVoice);

INCLUDE_ASM("asm/nonmatchings/libsnd/vm_pb", _SsVmPitchBend);
#else
// Written from libsnd 3.3's SpuVmPBVoice and SpuVmPitchBend.

// Bends the voice when it plays the sequence's program: bend is 0..127,
// 64 the centre, scaled to the tone's pbmax semitones up or pbmin down.
// Returns 1 when the voice matched.
short _SsVmPBVoice(short voice, short seq_sep_no, short vabId, short prog,
                   unsigned short bend) {
    short amount = bend - 0x40;
    VagAtr* tn;
    int note;
    int fine;
    int product;

    if (_svm_voice[voice].unke != seq_sep_no ||
        _svm_voice[voice].vabId != vabId || _svm_voice[voice].prog != prog) {
        return 0;
    }
    tn = &_svm_tn[(u16)(_svm_voice[voice].tone +
                        ((u8)_svm_cur.field_7_fake_program << 4))];
    note = (u16)_svm_voice[voice].note;
    if (amount > 0) {
        product = amount * tn->pbmax;
        note += product / 63;
        fine = (product % 63) * 2;
    } else if (amount < 0) {
        product = amount * tn->pbmin;
        note += product / 64 - 1;
        fine = (product % 64) * 2 + 0x7F;
    } else {
        fine = 0;
    }
    _svm_cur.voice = voice;
    _svm_cur.tone = (u8)_svm_voice[voice].tone;
    _svm_sreg_buf[voice].pitch = note2pitch2(note & 0xFFFF, fine & 0xFFFF);
    _svm_sreg_dirty[voice] |= 4;
    return 1;
}

// Bends every voice playing the sequence's program; returns how many.
int _SsVmPitchBend(
    short seq_sep_no, short vabId, short prog, unsigned short bend) {
    int count = 0;
    int i;

    _SsVmVSetUp(vabId, prog);
    _svm_cur.seq_sep_no = seq_sep_no;
    for (i = 0; i < _SsVmMaxVoice; i++) {
        count += _SsVmPBVoice(i, seq_sep_no, vabId, prog, bend);
    }
    return count;
}
#endif
