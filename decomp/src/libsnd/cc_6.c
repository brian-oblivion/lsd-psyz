#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_6", _SsContDataEntry);
#else
// CC6, the data entry. Right after a loop start it is the loop count. While
// the NRPN is not a loop's, it is the attribute, as CC98 is. Otherwise, once
// both RPN bytes are in, RPN 0 sets the pitch bend range of the program's
// tones; once both NRPN bytes are in, SsFCALL.ccentry sets the attribute of
// tone unk17 (every tone for 16). Written from libsnd 3.3's ContDataEntry,
// whose RPN 1 and 2 rewrite each tone unchanged.
void _SsContDataEntry(short seq_access_num, short seq_num, unsigned char data) {
    struct SeqStruct* score = &_ss_score[seq_access_num][seq_num];
    u8 prog = score->programs[score->channel];
    ProgAtr pg;
    VagAtr vag = {0};
    int i;

    SsUtGetProgAtr(score->unk26, prog, &pg);
    if (score->unk18 == 1 && score->unk10 == 0) {
        score->unk19 = data;
        score->unk10 = 1;
        score->unk90 = _SsReadDeltaValue(seq_access_num, seq_num);
        return;
    }
    if (score->unk17 != 30 && score->unk17 != 20) {
        score->unk16 = data;
        score->unk1B++;
        score->unk90 = _SsReadDeltaValue(seq_access_num, seq_num);
        return;
    }
    if (score->unk1A == 2) {
        if (score->unk15 == 0 && score->unk13 <= 2) {
            for (i = 0; i < pg.tones; i++) {
                SsUtGetVagAtr(score->unk26, prog, i, &vag);
                if (score->unk13 == 0) {
                    vag.pbmin = vag.pbmax = data & 0x7F;
                }
                SsUtSetVagAtr(score->unk26, prog, i, &vag);
            }
        }
        score->unk90 = _SsReadDeltaValue(seq_access_num, seq_num);
        score->unk1A = 0;
        return;
    }
    if (score->unk1B == 2) {
        if (score->unk16 < 20) {
            if (score->unk17 == 16) {
                for (i = 0; i < pg.tones; i++) {
                    SsFCALL.ccentry[score->unk16](
                        score->unk26, prog, i, vag, score->unk16, data);
                }
            } else {
                SsFCALL.ccentry[score->unk16](
                    score->unk26, prog, score->unk17, vag, score->unk16, data);
            }
        }
        score->unk90 = _SsReadDeltaValue(seq_access_num, seq_num);
        score->unk1B = 0;
        return;
    }
    score->unk90 = _SsReadDeltaValue(seq_access_num, seq_num);
}
#endif
