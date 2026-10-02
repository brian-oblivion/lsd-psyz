#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_100", _SsContRpn1);
#else
// CC100: the RPN's LSB.
void _SsContRpn1(short seq_access_num, short seq_num, unsigned char data) {
    struct SeqStruct* score = &_ss_score[seq_access_num][seq_num];

    score->unk13 = data;
    score->unk1A++;
    score->unk90 = _SsReadDeltaValue(seq_access_num, seq_num);
}
#endif
