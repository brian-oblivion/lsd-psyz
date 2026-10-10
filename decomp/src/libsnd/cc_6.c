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
    u8 prog = score->programs[score->channel_idx];
    ProgAtr pg;
    VagAtr vag = {0};
    int i;

    SsUtGetProgAtr(score->vab_id, prog, &pg);
    if (score->unk18 == 1 && score->unk10 == 0) {
        score->unk19 = data;
        score->unk10 = 1;
        score->delta_value = _SsReadDeltaValue(seq_access_num, seq_num);
        return;
    }
    if (score->unk1E != 30 && score->unk1E != 20) {
        score->unk1D = data;
        score->unk1B++;
        score->delta_value = _SsReadDeltaValue(seq_access_num, seq_num);
        return;
    }
    if (score->unk1F == 2) {
        if (score->unk15 == 0 && score->unk1C <= 2) {
            for (i = 0; i < pg.tones; i++) {
                SsUtGetVagAtr(score->vab_id, prog, i, &vag);
                if (score->unk1C == 0) {
                    vag.pbmin = vag.pbmax = data & 0x7F;
                }
                SsUtSetVagAtr(score->vab_id, prog, i, &vag);
            }
        }
        score->delta_value = _SsReadDeltaValue(seq_access_num, seq_num);
        score->unk1F = 0;
        return;
    }
    if (score->unk1B == 2) {
        if (score->unk1D < 20) {
            if (score->unk1E == 16) {
                for (i = 0; i < pg.tones; i++) {
                    SsFCALL.ccentry[score->unk1D](
                        score->vab_id, prog, i, vag, score->unk1D, data);
                }
            } else {
                SsFCALL.ccentry[score->unk1D](
                    score->vab_id, prog, score->unk1E, vag, score->unk1D, data);
            }
        }
        score->delta_value = _SsReadDeltaValue(seq_access_num, seq_num);
        score->unk1B = 0;
        return;
    }
    score->delta_value = _SsReadDeltaValue(seq_access_num, seq_num);
}
#endif
