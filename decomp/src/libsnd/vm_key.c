#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/vm_key", _SsVmKeyOn);

INCLUDE_ASM("asm/nonmatchings/libsnd/vm_key", _SsVmKeyOff);

INCLUDE_ASM("asm/nonmatchings/libsnd/vm_key", _SsVmSeKeyOn);

INCLUDE_ASM("asm/nonmatchings/libsnd/vm_key", _SsVmSeKeyOff);

INCLUDE_ASM("asm/nonmatchings/libsnd/vm_key", KeyOnCheck);
#else
// Written from libsnd 3.3's SpuVmKeyOn, SpuVmKeyOff, SpuVmSeKeyOn and
// SpuVmSeKeyOff.

// Keys a note of a program on: one voice for each of the program's tones
// whose note range holds the note. vol 0 keys the note off instead. Returns
// the voices keyed, four bits each.
int _SsVmKeyOn(int seq_sep_no, short vabId, short prog, unsigned short note,
               unsigned short vol, unsigned short pan) {
    struct SeqStruct* score = NULL;
    ProgAtr* pg;
    VagAtr* tn;
    u8 vags[128];
    u8 tones[128];
    int count;
    int voices;
    int voice;
    int i;

    if ((short)seq_sep_no != 0x21) {
        score = &_ss_score[seq_sep_no & 0xFF][(seq_sep_no >> 8) & 0xFF];
    }
    if (_SsVmVSetUp(vabId, prog)) {
        return -1;
    }
    pg = &_svm_pg[prog];
    _svm_cur.seq_sep_no = seq_sep_no;
    _svm_cur.note = note;
    _svm_cur.fine = 0;
    _svm_cur.volume = vol;
    _svm_cur.pan = pan;
    _svm_cur.mvol = pg->mvol;
    _svm_cur.mpan = pg->mpan;
    _svm_cur.prog_tones = pg->tones;
    if ((u8)_svm_cur.fake_program >= _svm_vh->ps) {
        return -1;
    }
    if (vol == 0) {
        _SsVmKeyOff(seq_sep_no, vabId, prog, note);
        return 0;
    }

    count = 0;
    for (i = 0; i < (u8)_svm_cur.prog_tones; i++) {
        tn = &_svm_tn[(u8)_svm_cur.fake_program * 16 + i];
        if ((u8)note < tn->min || tn->max < (u8)note) {
            continue;
        }
        vags[count] = tn->vag;
        tones[count] = i;
        count++;
    }

    voices = 0;
    for (i = 0; i < count; i++) {
        _svm_cur.tone_vag_idx = vags[i];
        _svm_cur.tone = tones[i];
        tn = &_svm_tn[(u8)_svm_cur.fake_program * 16 + tones[i]];
        _svm_cur.tone_prior = tn->prior;
        _svm_cur.tone_vol = tn->vol;
        _svm_cur.tone_pan = tn->pan;
        _svm_cur.tone_center = tn->center;
        _svm_cur.tone_shift = tn->shift;
        _svm_cur.tone_mode = tn->mode;
        _svm_cur.tone_min = tn->min;
        _svm_cur.tone_max = tn->max;

        voice = (u8)_SsVmAlloc(0);
        _svm_cur.voice = voice;
        if (voice >= _SsVmMaxVoice) {
            continue;
        }
        _svm_voice[voice].unk1b = 1;
        _svm_voice[voice].unk2 = 0;
        _svm_voice[voice].seq_sep_no = seq_sep_no;
        _svm_voice[voice].vabId = _svm_cur.vabId;
        _svm_voice[voice].fake_program = (u8)_svm_cur.fake_program;
        _svm_voice[voice].prog = prog;
        if (score != NULL) {
            // NoteOn scaled the velocity by the channel volume, which is not
            // 0 here, or vol would be 0.
            _svm_voice[voice].voll1 = vol * 127 / score->vol[score->channel_idx];
        }
        _svm_voice[voice].pan = pan;
        _svm_voice[voice].tone = tones[i];
        _svm_voice[voice].note = note;
        _svm_voice[voice].priority = (u8)_svm_cur.tone_prior;
        _svm_voice[voice].vag_idx = vags[i];

        _SsVmDoAllocate();
        if (vags[i] == 0xFF) {
            vmNoiseOn(voice);
        } else {
            _SsVmKeyOnNow(count, note2pitch());
        }
        voices = (voices << 4) | voice;
    }
    return voices;
}

// Keys off every voice playing the note of the program for the sequence;
// returns how many.
int _SsVmKeyOff(int seq_sep_no, short vabId, short prog, unsigned short note) {
    int count = 0;
    int i;

    for (i = 0; i < _SsVmMaxVoice; i++) {
        if (_svm_voice[i].note != (short)note || _svm_voice[i].prog != prog ||
            _svm_voice[i].seq_sep_no != (short)seq_sep_no ||
            _svm_voice[i].vabId != vabId) {
            continue;
        }
        if (_svm_voice[i].vag_idx == 0xFF) {
            vmNoiseOff(i);
        } else {
            _svm_cur.voice = i;
            _SsVmKeyOffNow(0);
        }
        count++;
    }
    return count;
}

// A sound effect's key-on: one volume and a pan from the two volumes.
int _SsVmSeKeyOn(short vabId, short prog, unsigned short note, int pitch,
                 unsigned short voll, unsigned short volr) {
    unsigned short vol;
    unsigned short pan;

    if (voll == volr) {
        pan = 64;
        vol = voll;
    } else if (volr < voll) {
        vol = voll;
        pan = (volr << 6) / voll;
    } else {
        vol = volr;
        pan = 127 - ((voll << 6) / volr);
    }
    return _SsVmKeyOn(0x21, vabId, prog, note, vol, pan);
}

int _SsVmSeKeyOff(short vabId, short prog, unsigned short note) {
    return _SsVmKeyOff(0x21, vabId, prog, note);
}

void KeyOnCheck(void) {}
#endif
