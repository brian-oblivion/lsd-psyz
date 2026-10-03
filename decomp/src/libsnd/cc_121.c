#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_121", _SsContResetAll);
#else
// CC121: the channel's program, volume and pan back to their defaults, and
// the reverb and damper off.
void _SsContResetAll(short seq_access_num, short seq_num) {
    struct SeqStruct* score = &_ss_score[seq_access_num][seq_num];

    SsUtReverbOff();
    _SsVmDamperOff();
    score->programs[score->channel] = score->channel;
    score->unk13 = 0;
    score->unk15 = 0;
    score->vol[score->channel] = 0x7F;
    score->panpot[score->channel] = 0x40;
    score->unk90 = _SsReadDeltaValue(seq_access_num, seq_num);
}
#endif
