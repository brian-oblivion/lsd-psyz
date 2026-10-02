#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/vm_autov", SeAutoVol);

INCLUDE_ASM("asm/nonmatchings/libsnd/vm_autov", SetAutoVol);
#else
// Written from libsnd 3.3's SeAutoVol and SetAutoVol.

// Starts a volume ramp on the voice from start_vol to end_vol over
// delta_time ticks, which _SsVmFlush steps through SetAutoVol.
void SeAutoVol(short voice, short start_vol, short end_vol, short delta_time) {
    short diff = start_vol - end_vol;
    short step;

    if (start_vol == end_vol) {
        return;
    }
    _svm_voice[voice].auto_vol = 1;
    _svm_voice[voice].start_vol = start_vol;
    _svm_voice[voice].end_vol = end_vol;
    if ((diff < 0 ? -diff : diff) < delta_time) {
        step = delta_time / diff;
        _svm_voice[voice].unk1e = 1;
        _svm_voice[voice].unk20 = step;
        _svm_voice[voice].unk22 = step;
    } else {
        step = diff / delta_time;
        _svm_voice[voice].unk20 = 0;
        _svm_voice[voice].unk1e = step;
    }
}

// Steps the voice's volume ramp once: every unk20 ticks the volume moves by
// unk1e, and reaching end_vol ends the ramp. The volume becomes _svm_cur's,
// and the voice's levels are recomputed from it with _svm_cur's program and
// tone volumes and pans, as retail does.
void SetAutoVol(short voice) {
    struct SpuVoice* v = &_svm_voice[voice];
    unsigned level, l, r;
    unsigned p;

    if (v->unk20 != 0) {
        if (v->unk22-- > 0) {
            return;
        }
        v->unk22 = v->unk20;
    }
    v->start_vol += v->unk1e;
    if (v->unk1e > 0) {
        if (v->start_vol >= v->end_vol) {
            v->start_vol = v->end_vol;
            v->auto_vol = 0;
        }
    } else if (v->unk1e < 0) {
        if (v->start_vol <= v->end_vol) {
            v->start_vol = v->end_vol;
            v->auto_vol = 0;
        }
    }

    _svm_cur.volume = v->start_vol;
    level = v->start_vol * (_svm_vh->mvol * 0x3FFF) / 16129;
    level = level * (u8)_svm_cur.mvol * (u8)_svm_cur.tone_vol / 16129;
    l = r = level & 0xFFFF;

    p = (u8)_svm_cur.tone_pan;
    if (p < 0x40) {
        r = (r * p) >> 6;
    } else {
        l = (l * (0x7F - p)) >> 6;
    }
    p = (u8)_svm_cur.mpan;
    if (p < 0x40) {
        r = (u16)(r * p / 64);
    } else {
        l = (u16)(l * (0x7F - p) / 64);
    }
    p = (u8)_svm_cur.pan;
    if (p < 0x40) {
        r = (u16)(r * p / 64);
    } else {
        l = (u16)(l * (0x7F - p) / 64);
    }
    if (_svm_stereo_mono == 1) {
        if (r > l) {
            l = r;
        } else {
            r = l;
        }
    }
    _svm_sreg_buf[voice].volume.right = r;
    _svm_sreg_buf[voice].volume.left = l;
    _svm_sreg_dirty[voice] |= 3;
}
#endif
