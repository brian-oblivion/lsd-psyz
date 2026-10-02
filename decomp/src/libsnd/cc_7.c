#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_7", _SsContMainVol);
#else
// CC7: the channel's volume, applied to the voices playing.
void _SsContMainVol(short seq_access_num, short seq_num, unsigned char data) {
    struct SeqStruct* score = &_ss_score[seq_access_num][seq_num];
    u8 ch = score->channel;

    _SsVmSetVol((seq_num << 8) | seq_access_num, score->unk26,
                score->programs[ch], data, score->panpot[ch]);
    score->vol[ch] = data;
    score->unk90 = _SsReadDeltaValue(seq_access_num, seq_num);
}
#endif
