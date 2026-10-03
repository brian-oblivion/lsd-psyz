#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/next", _SsSndNextSep);
#else
// Starts the SEP's next sequence, once, from its start. Written from libsnd
// 3.3's.
void _SsSndNextSep(short sep_access_num, short seq_num) {
    struct SeqStruct* score = &_ss_score[sep_access_num][seq_num];

    score->unk20 = 1;
    score->unk21 = 0;
    score->flags &=
        ~(SEQ_FLAG_100 | SEQ_FLAG_8 | SEQ_FLAG_2 | SEQ_FLAG_4 | SEQ_FLAG_200);
    score->play_mode = 1;
    score->unk0 = score->read_pos;
    score->flags |= SEQ_FLAG_1;
}
#endif
