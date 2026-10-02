#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/ssdecres", _SsSndSetDecres);

INCLUDE_ASM("asm/nonmatchings/libsnd/ssdecres", SsSeqSetDecrescendo);

INCLUDE_ASM("asm/nonmatchings/libsnd/ssdecres", SsSepSetDecrescendo);
#else
static void _SsSndSetDecres(
    short sep_access_num, short seq_num, short vol, long v_time) {
    _SsSndSetVolData(sep_access_num, seq_num, vol, v_time);
    _ss_score[sep_access_num][seq_num].flags |= SEQ_FLAG_20;
    _ss_score[sep_access_num][seq_num].flags &= ~SEQ_FLAG_10;
}

void SsSeqSetDecrescendo(short seq_access_num, short vol, long v_time) {
    _SsSndSetDecres(seq_access_num, 0, vol, v_time);
}

void SsSepSetDecrescendo(
    short sep_access_num, short seq_num, short vol, long v_time) {
    _SsSndSetDecres(sep_access_num, seq_num, vol, v_time);
}
#endif
