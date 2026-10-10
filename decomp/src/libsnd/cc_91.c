#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_91", _SsContExternal);
#else
// CC91: the reverb depth.
void _SsContExternal(short seq_access_num, short seq_num, unsigned char data) {
    struct SeqStruct* score = &_ss_score[seq_access_num][seq_num];

    SsUtSetReverbDepth(data, data);
    score->delta_value = _SsReadDeltaValue(seq_access_num, seq_num);
}
#endif
