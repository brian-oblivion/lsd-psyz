#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/ssreplay", _SsSndSetReplayMode);

INCLUDE_ASM("asm/nonmatchings/libsnd/ssreplay", SsSeqReplay);

INCLUDE_ASM("asm/nonmatchings/libsnd/ssreplay", SsSepReplay);
#else
// Written from libsnd 3.3's. Nothing for a sequence that has stopped or is
// stopping.
void _SsSndSetReplayMode(short sep_access_num, short seq_num) {
    struct SeqStruct* score = &_ss_score[sep_access_num][seq_num];

    if (score->flags & (SEQ_FLAG_200 | SEQ_FLAG_4) ||
        score->flags & SEQ_FLAG_100) {
        return;
    }
    score->flags &= ~SEQ_FLAG_2;
    score->flags |= SEQ_FLAG_8;
    score->flags |= SEQ_FLAG_1;
}

void SsSeqReplay(short seq_access_num) {
    _SsSndSetReplayMode(seq_access_num, 0);
}

void SsSepReplay(short sep_access_num, short seq_num) {
    _SsSndSetReplayMode(sep_access_num, seq_num);
}
#endif
