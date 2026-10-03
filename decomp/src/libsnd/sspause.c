#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/sspause", _SsSndSetPauseMode);

INCLUDE_ASM("asm/nonmatchings/libsnd/sspause", SsSeqPause);

INCLUDE_ASM("asm/nonmatchings/libsnd/sspause", SsSepPause);
#else
// Written from libsnd 3.3's. The pause takes effect at the next tick
// (_SsSndPause), which keys the sequence's voices off.
void _SsSndSetPauseMode(short sep_access_num, short seq_num) {
    struct SeqStruct* score = &_ss_score[sep_access_num][seq_num];

    _SsVmGetSeqVol(
        sep_access_num | (seq_num << 8), &score->unk5C, &score->unk5E);
    score->flags &= ~SEQ_FLAG_1;
    score->flags &= ~SEQ_FLAG_8;
    score->flags |= SEQ_FLAG_2;
}

void SsSeqPause(short seq_access_num) { _SsSndSetPauseMode(seq_access_num, 0); }

void SsSepPause(short sep_access_num, short seq_num) {
    _SsSndSetPauseMode(sep_access_num, seq_num);
}
#endif
