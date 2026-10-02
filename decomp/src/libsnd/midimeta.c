#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/midimeta", _SsGetMetaEvent);
#else
// Written from libsnd 3.3's GetMetaEvent. Two meta events are understood:
// Set Tempo (0x51) recomputes the ticks per call from the new tempo, and End
// of Track (0x2F) rewinds while plays are left (unk20 0 plays forever), and
// otherwise stops the sequence: SsSeqCalledTbyT sees flag 4 and stops it.
void _SsGetMetaEvent(short seq_access_num, short seq_num, unsigned char type) {
    struct SeqStruct* score = &_ss_score[seq_access_num][seq_num];

    if (type == 0x51) {
        unsigned base = VBLANK_MINUS * 15;
        unsigned divisor = base * 4;
        unsigned ticks;
        int usec;

        usec = *score->unk0++ << 16;
        usec |= *score->unk0++ << 8;
        usec |= *score->unk0++;
        score->unk94 = 60000000 / usec;
        ticks = score->unk50 * score->unk94 * 10;
        if (ticks < divisor) {
            score->unk52 = (VBLANK_MINUS * 600) / (score->unk50 * score->unk94);
            score->unk54 = score->unk52;
        } else {
            score->unk52 = -1;
            score->unk54 = ticks / divisor;
            if (base * 2 < ticks % divisor) {
                score->unk54++;
            }
        }
        score->unk90 = _SsReadDeltaValue(seq_access_num, seq_num);
        return;
    }
    if (type != 0x2F) {
        return;
    }
    score->unk21++;
    if (score->unk20 == 0 || score->unk21 < score->unk20) {
        score->delta_value = 0;
        score->unk18 = 0;
        score->unk90 = 0;
        score->unk0 = score->read_pos;
        if (score->unk20 != 0) {
            score->loop_pos = score->read_pos;
        }
        return;
    }
    score->flags &= ~SEQ_FLAG_1;
    score->flags &= ~SEQ_FLAG_8;
    score->flags &= ~SEQ_FLAG_2;
    score->flags |= SEQ_FLAG_200;
    score->flags |= SEQ_FLAG_4;
    score->loop_pos = score->read_pos;
    score->play_mode = 0;
    if (score->unk22 != 0xFF) {
        _SsSndNextSep(score->unk22, score->unk23);
        score->play_mode = 0;
    }
    _SsVmSeqKeyOff((seq_num << 8) | seq_access_num);
    score->unk90 = score->unk54;
}
#endif
