#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/seqinit", _SsInitSoundSeq);
#else
// Sets up access number flag's score from the SEQ at addr: the "pQES" header
// (version 1), the resolution and tempo, then the first delta time, and from
// those the ticks per call (see _SsSeqPlay). Returns 0, or -1 for a SEQ of
// another version. Written from libsnd 3.3's.
int _SsInitSoundSeq(short flag, short vab_id, u_long* addr) {
    struct SeqStruct* score = &_ss_score[flag][0];
    u8* p = (u8*)addr;
    int usec, bpm, rem;
    unsigned int ticks;
    int ch;

    score->unk26 = vab_id;
    score->unk50 = 0;
    score->unk10 = 0;
    score->unk11 = 0;
    score->channel = 0;
    score->unk13 = 0;
    score->unk15 = 0;
    score->unk16 = 0;
    score->unk17 = 0;
    score->unk18 = 0;
    score->unk19 = 0;
    score->unk1A = 0;
    score->unk1B = 0;
    score->play_mode = 0;
    score->unk21 = 0;
    score->unk56 = 0;
    score->unk84 = 0;
    score->delta_value = 0;
    score->unk8c = 0;
    score->unk90 = 0;
    score->unk47 = 0x7F;
    for (ch = 0; ch < 16; ch++) {
        score->programs[ch] = ch;
        score->panpot[ch] = 0x40;
        score->vol[ch] = 0x7F;
    }
    score->unk52 = 1;

    score->unk0 = p;
    if (p[0] != 'S' && p[0] != 'p') {
        printf("This is an old SEQ Data Format.\n");
        return 0;
    }
    score->unk0 = p + 8;
    if (p[7] != 1) {
        printf("This is not SEQ Data.\n");
        return -1;
    }
    score->unk50 = (p[8] << 8) | p[9];
    usec = (p[10] << 16) | (p[11] << 8) | p[12];
    bpm = 60000000 / usec;
    rem = 60000000 % usec;
    score->unk8c = (usec >> 1) < rem ? bpm + 1 : bpm;
    score->unk94 = score->unk8c;
    score->unk0 = p + 15; // past the rhythm, two bytes

    score->unk84 = score->unk90 = _SsReadDeltaValue(flag, 0);
    score->loop_pos = score->unk0;
    score->read_pos = score->unk0;
    score->next_sep_pos = score->unk0;

    ticks = score->unk50 * score->unk8c;
    if (ticks * 10 < VBLANK_MINUS * 60) {
        score->unk52 = score->unk54 = (VBLANK_MINUS * 600) / ticks;
    } else {
        score->unk52 = -1;
        score->unk54 = (ticks * 10) / (VBLANK_MINUS * 60);
        if ((VBLANK_MINUS * 30) < (ticks * 10) % (VBLANK_MINUS * 60)) {
            score->unk54++;
        }
    }
    score->unk56 = score->unk54;
    return 0;
}
#endif
