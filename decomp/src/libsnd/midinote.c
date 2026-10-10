#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/midinote", _SsNoteOn);
#else
// Keys the note on at the velocity scaled by the channel volume, or off for
// velocity 0; nothing while the sequence's left volume is 0. Written from
// libsnd 3.3's NoteOn.
void _SsNoteOn(short seq_access_num, short seq_num, unsigned char note,
               unsigned char vel) {
    struct SeqStruct* score = &_ss_score[seq_access_num][seq_num];
    u8 ch = score->channel_idx;
    int seq_sep_no = (seq_num << 8) | seq_access_num;
    unsigned short vol = vel * score->vol[ch] / 127;

    if (score->voll == 0) {
        return;
    }
    if (vel != 0) {
        _SsVmKeyOn(seq_sep_no, score->vab_id, score->programs[ch], note, vol,
                   score->panpot[ch]);
        score->unk47 = vel;
    } else {
        _SsVmKeyOff(seq_sep_no, score->vab_id, score->programs[ch], note);
    }
}
#endif
