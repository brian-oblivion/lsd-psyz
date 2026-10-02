#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_0", _SsContBankChange);
#else
// CC0: the VAB the sequence plays.
void _SsContBankChange(
    short seq_access_num, short seq_num, unsigned char data) {
    struct SeqStruct* score = &_ss_score[seq_access_num][seq_num];

    score->unk26 = data;
    score->unk90 = _SsReadDeltaValue(seq_access_num, seq_num);
}
#endif
