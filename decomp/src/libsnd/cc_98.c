#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_98", _SsContNrpn1);
#else
// CC98, the NRPN's LSB: the loop count right after a loop start, else the
// data entry's attribute (unless the NRPN is a loop's). Under NRPN 40 the
// value also goes to the mark callback.
void _SsContNrpn1(short seq_access_num, short seq_num, unsigned char data) {
    struct SeqStruct* score = &_ss_score[seq_access_num][seq_num];

    if (score->unk18 == 1 && score->unk10 == 0) {
        score->unk19 = data;
        score->unk10 = 1;
    } else if (score->unk1E != 30 && score->unk1E != 20) {
        score->unk1D = data;
        score->unk1B++;
    }
    if (score->unk1E == 40 &&
        _SsMarkCallback[seq_access_num][seq_num] != NULL) {
        _SsMarkCallback[seq_access_num][seq_num](seq_access_num, seq_num, data);
    }
    score->delta_value = _SsReadDeltaValue(seq_access_num, seq_num);
}
#endif
