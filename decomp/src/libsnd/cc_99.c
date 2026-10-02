#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_99", _SsContNrpn2);
#else
// CC99, the NRPN's MSB: 20 marks a loop start at the next event, 30 a loop
// end, which jumps back while the count lasts (127 and up: forever).
void _SsContNrpn2(short seq_access_num, short seq_num, unsigned char data) {
    struct SeqStruct* score = &_ss_score[seq_access_num][seq_num];

    score->unk17 = data;
    switch (data) {
    case 20:
        score->unk18 = 1;
        score->unk90 = _SsReadDeltaValue(seq_access_num, seq_num);
        score->loop_pos = score->unk0;
        return;
    case 30:
        if (score->unk19 == 0) {
            score->unk10 = 0;
            score->unk90 = _SsReadDeltaValue(seq_access_num, seq_num);
            return;
        }
        if (score->unk19 < 0x7F) {
            score->unk19--;
            score->unk90 = _SsReadDeltaValue(seq_access_num, seq_num);
            if (score->unk19 != 0) {
                score->unk0 = score->loop_pos;
            } else {
                score->unk10 = 0;
            }
            return;
        }
        _SsReadDeltaValue(seq_access_num, seq_num);
        score->unk0 = score->loop_pos;
        score->unk90 = 0;
        return;
    default:
        score->unk1B++;
        score->unk90 = _SsReadDeltaValue(seq_access_num, seq_num);
        return;
    }
}
#endif
