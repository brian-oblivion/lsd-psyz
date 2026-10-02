#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/midicc", _SsSetControlChange);
#else
// Reads the controller's value and hands it to SsFCALL's handler for the
// controller, which reads the delta time; the rest only read it. Written
// from libsnd 3.3's _SsSetControlChange, which also handles CC65
// (portamento) by setting the program's tones' mode: kept here.

static void ContPortamento(short seq_access_num, short seq_num, u8 data) {
    struct SeqStruct* score = &_ss_score[seq_access_num][seq_num];
    u8 prog = score->programs[score->channel];
    ProgAtr pg;
    VagAtr vag;
    int i;

    SsUtGetProgAtr(score->unk26, prog, &pg);
    for (i = 0; i < pg.tones; i++) {
        SsUtGetVagAtr(score->unk26, prog, i, &vag);
        if (data < 0x40) {
            vag.mode = 2;
        } else if (data < 0x80) {
            vag.mode = 0;
        }
        SsUtSetVagAtr(score->unk26, prog, i, &vag);
    }
    score->unk90 = _SsReadDeltaValue(seq_access_num, seq_num);
}

void _SsSetControlChange(
    short seq_access_num, short seq_num, unsigned char control) {
    struct SeqStruct* score = &_ss_score[seq_access_num][seq_num];
    u8 data = *score->unk0++;

    switch (control) {
    case 0:
        SsFCALL.control[CC_BANKCHANGE](seq_access_num, seq_num, data);
        return;
    case 6:
        SsFCALL.control[CC_DATAENTRY](seq_access_num, seq_num, data);
        return;
    case 7:
        SsFCALL.control[CC_MAINVOL](seq_access_num, seq_num, data);
        return;
    case 10:
        SsFCALL.control[CC_PANPOT](seq_access_num, seq_num, data);
        return;
    case 11:
        SsFCALL.control[CC_EXPRESSION](seq_access_num, seq_num, data);
        return;
    case 64:
        SsFCALL.control[CC_DAMPER](seq_access_num, seq_num, data);
        return;
    case 65:
        ContPortamento(seq_access_num, seq_num, data);
        return;
    case 91:
        SsFCALL.control[CC_EXTERNAL](seq_access_num, seq_num, data);
        return;
    case 98:
        SsFCALL.control[CC_NRPN1](seq_access_num, seq_num, data);
        return;
    case 99:
        SsFCALL.control[CC_NRPN2](seq_access_num, seq_num, data);
        return;
    case 100:
        SsFCALL.control[CC_RPN1](seq_access_num, seq_num, data);
        return;
    case 101:
        SsFCALL.control[CC_RPN2](seq_access_num, seq_num, data);
        return;
    case 121:
        SsFCALL.control[CC_RESETALL](seq_access_num, seq_num);
        return;
    }
    score->unk90 = _SsReadDeltaValue(seq_access_num, seq_num);
}
#endif
