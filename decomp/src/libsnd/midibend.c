#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/midibend", _SsSetPitchBend);
#else
// The event's second data byte is the bend, 0x40 the centre; the first was
// skipped. Written from libsnd 3.3's SetPitchBend.
void _SsSetPitchBend(short seq_access_num, short seq_num) {
    struct SeqStruct* score = &_ss_score[seq_access_num][seq_num];
    u8 bend = *score->unk0++;

    _SsVmPitchBend((seq_num << 8) | seq_access_num, score->unk26,
                   score->programs[score->channel], bend);
    score->unk90 = _SsReadDeltaValue(seq_access_num, seq_num);
}
#endif
