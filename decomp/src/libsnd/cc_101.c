#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_101", _SsContRpn2);
#else
// CC101: the RPN's MSB.
void _SsContRpn2(short seq_access_num, short seq_num, unsigned char data) {
    struct SeqStruct* score = &_ss_score[seq_access_num][seq_num];

    score->unk15 = data;
    score->unk1A++;
    score->unk90 = _SsReadDeltaValue(seq_access_num, seq_num);
}
#endif
