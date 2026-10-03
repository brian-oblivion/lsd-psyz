#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_11", _SsContExpression);
#else
// CC11: the program's volume.
void _SsContExpression(
    short seq_access_num, short seq_num, unsigned char data) {
    struct SeqStruct* score = &_ss_score[seq_access_num][seq_num];
    u8 ch = score->channel;

    _SsVmSetProgVol(score->unk26, score->programs[ch], data);
    _SsVmSetVol((seq_num << 8) | seq_access_num, score->unk26,
                score->programs[ch], score->vol[ch], score->panpot[ch]);
    score->unk90 = _SsReadDeltaValue(seq_access_num, seq_num);
}
#endif
