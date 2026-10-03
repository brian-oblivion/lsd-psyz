#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/vm_autop", SeAutoPan);
#else
// Starts a pan ramp on the voice from start_pan to end_pan over delta_time
// ticks, which _SsVmFlush steps through SetAutoPan. Written from libsnd 3.3.
void SeAutoPan(short voice, short start_pan, short end_pan, short delta_time) {
    short diff = start_pan - end_pan;
    short step;

    if (start_pan == end_pan) {
        return;
    }
    _svm_voice[voice].auto_pan = 1;
    _svm_voice[voice].start_pan = start_pan;
    _svm_voice[voice].end_pan = end_pan;
    if ((diff < 0 ? -diff : diff) < delta_time) {
        step = delta_time / diff;
        _svm_voice[voice].unk2a = 1;
        _svm_voice[voice].unk2c = step;
        _svm_voice[voice].unk2e = step;
    } else {
        step = diff / delta_time;
        _svm_voice[voice].unk2c = 0;
        _svm_voice[voice].unk2a = step;
    }
}
#endif

void SetAutoPan(short voice) {
    unsigned voll_t, volr_t;
    unsigned short voll, volr;
    short pos;
    unsigned char pan;
    unsigned short pan2;
    int mvol_scaled;

    pos = voice * 8;
    if (_svm_voice[voice].unk2e-- > 0) {
        return;
    }
    _svm_voice[voice].start_pan += _svm_voice[voice].unk2a;
    if (_svm_voice[voice].unk2a > 0) {
        if (_svm_voice[voice].start_pan >= _svm_voice[voice].end_pan) {
            _svm_voice[voice].start_pan = _svm_voice[voice].end_pan;
            _svm_voice[voice].auto_pan = 0;
        }
    } else if (_svm_voice[voice].unk2a < 0) {
        if (_svm_voice[voice].start_pan <= _svm_voice[voice].end_pan) {
            _svm_voice[voice].start_pan = _svm_voice[voice].end_pan;
            _svm_voice[voice].auto_pan = 0;
        }
    }
    pan = _svm_voice[voice].start_pan;
    _svm_cur.pan = pan;
    mvol_scaled = _svm_vh->mvol * 0x3FFF;

    voll_t = ((_svm_cur.volume * mvol_scaled) / 0x7F) / 0x7F;
    volr_t = ((_svm_cur.volume * mvol_scaled) / 0x7F) / 0x7F;

    voll_t = ((voll_t * _svm_cur.mvol * _svm_cur.tone_vol) / 0x7F) / 0x7F;
    volr_t = ((volr_t * _svm_cur.mvol * _svm_cur.tone_vol) / 0x7F) / 0x7F;

    if (_svm_cur.tone_pan < 0x40) {
        voll = voll_t;
        volr = (volr_t * _svm_cur.tone_pan) / 0x40;
    } else {
        voll = (voll_t * (0x7F - _svm_cur.tone_pan)) / 0x40;
        volr = volr_t;
    }

    if (_svm_cur.mpan < 0x40) {
        volr = (volr * _svm_cur.mpan) / 0x40;
    } else {
        voll = (voll * (0x7F - _svm_cur.mpan)) / 0x40;
    }
    pan2 = pan;
    if (pan2 < 0x40) {
        volr = (volr * pan2) / 0x40;
    } else {
        voll = (voll * (0x7F - pan2)) / 0x40;
    }

    if (_svm_stereo_mono == 1) {
        if (volr > voll) {
            voll = volr;
        } else {
            volr = voll;
        }
    }
    _svm_voice[voice].unk2e = _svm_voice[voice].unk2c;
    ((short*)_svm_sreg_buf)[pos + 0] = voll;
    ((short*)_svm_sreg_buf)[pos + 1] = volr;
    _svm_sreg_dirty[voice] |= 3;
}
