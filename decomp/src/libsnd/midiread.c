#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/midiread", _SsSeqPlay);

INCLUDE_ASM("asm/nonmatchings/libsnd/midiread", _SsSeqGetEof);

INCLUDE_ASM("asm/nonmatchings/libsnd/midiread", _SsGetSeqData);
#else
// Written from libsnd 3.3's SeqPlay and GetSeqData.

// Plays one tick of the sequence. unk90 holds the ticks left to the next
// event and unk54 the ticks a call covers; when a call covers less than a
// tick (unk52 >= 0), unk52 counts the calls down to the next tick.
void _SsSeqPlay(short seq_access_num, short seq_num) {
    struct SeqStruct* score = &_ss_score[seq_access_num][seq_num];
    short ticks = score->unk54;
    int left = score->unk90;
    int sum;

    if (left - ticks > 0) {
        if (score->unk52 > 0) {
            score->unk52--;
        } else if (score->unk52 == 0) {
            score->unk52 = ticks;
            score->unk90--;
        } else {
            score->unk90 = left - ticks;
        }
        return;
    }
    sum = left;
    for (;;) {
        _SsGetSeqData(seq_access_num, seq_num);
        if (score->unk90 == 0) {
            continue;
        }
        sum += score->unk90;
        if (sum >= score->unk54) {
            score->unk90 = sum - score->unk54;
            return;
        }
    }
}

// Decodes the event at the read position and hands it to SsFCALL. A status
// byte sets the channel and the running status (0xFF for meta events, 0xF0
// to 0xFF); a data byte repeats the running status' event. Note-on events
// read their delta time here, the others in their handlers.
void _SsGetSeqData(short seq_access_num, short seq_num) {
    struct SeqStruct* score = &_ss_score[seq_access_num][seq_num];
    u8 status;
    u8 data;
    u8 vel;

    status = *score->unk0++;
    if (status & 0x80) {
        score->channel = status & 0xF;
        switch (status & 0xF0) {
        case 0x90:
            score->unk11 = 0x90;
            data = *score->unk0++;
            vel = *score->unk0++;
            score->unk90 = _SsReadDeltaValue(seq_access_num, seq_num);
            SsFCALL.noteon(seq_access_num, seq_num, data, vel);
            break;
        case 0xB0:
            score->unk11 = 0xB0;
            data = *score->unk0++;
            SsFCALL.control[CC_NUMBER](seq_access_num, seq_num, data);
            break;
        case 0xC0:
            score->unk11 = 0xC0;
            data = *score->unk0++;
            SsFCALL.programchange(seq_access_num, seq_num, data);
            break;
        case 0xE0:
            score->unk11 = 0xE0;
            score->unk0++;
            SsFCALL.pitchbend(seq_access_num, seq_num);
            break;
        case 0xF0:
            score->unk11 = 0xFF;
            data = *score->unk0++;
            SsFCALL.metaevent(seq_access_num, seq_num, data);
            break;
        }
        return;
    }
    switch (score->unk11) {
    case 0x90:
        vel = *score->unk0++;
        score->unk90 = _SsReadDeltaValue(seq_access_num, seq_num);
        SsFCALL.noteon(seq_access_num, seq_num, status, vel);
        break;
    case 0xB0:
        SsFCALL.control[CC_NUMBER](seq_access_num, seq_num, status);
        break;
    case 0xC0:
        SsFCALL.programchange(seq_access_num, seq_num, status);
        break;
    case 0xE0:
        SsFCALL.pitchbend(seq_access_num, seq_num);
        break;
    case 0xFF:
        SsFCALL.metaevent(seq_access_num, seq_num, status);
        break;
    }
}
#endif
