#include "libsnd_private.h"

#if !defined(__psyz) || !defined(PSYZ_LIBSND_33)
short _SsVmSetSeqVol(short seq_sep_no, u16 voll, u16 volr, short arg3) {
    struct SeqStruct* score;
    int vol;
    int vol_scaled;
    int tone;
    unsigned voll_t, volr_t;
    u8 pan;
    short i;

    score = &_ss_score[seq_sep_no & 0xFF][(seq_sep_no & 0xFF00) >> 8];
    _svm_cur.seq_sep_no = seq_sep_no;
    if (voll == 0) {
        voll = 1;
    }
    if (volr == 0) {
        volr = 1;
    }
    score->voll = voll;
    score->volr = volr;
    if (score->voll > 127) {
        score->voll = 127;
    }
    if (score->volr > 127) {
        score->volr = 127;
    }
    if (arg3 == 1) {
        for (i = 0; i < _SsVmMaxVoice; i++) {
            if ((u16)_svm_voice[i].seq_sep_no == (u16)seq_sep_no) {
                vol =
                    _svm_voice[i].voll1 * score->vol[score->channel_idx] / 127;
                vol_scaled = vol * 0x3FFF;
                voll_t = _svm_vh->mvol * vol_scaled / 0x3F01;
                tone = _svm_voice[i].fake_program * 16 + _svm_voice[i].tone;
                volr_t = voll_t * _svm_pg[_svm_voice[i].prog].mvol *
                         _svm_tn[tone].vol / 0x3F01;
                voll_t = volr_t * score->voll / 127;
                volr_t = volr_t * score->volr / 127;
                pan = _svm_tn[tone].pan;
                if (pan < 64) {
                    voll = voll_t;
                    volr = (volr_t * pan) / 63;
                } else {
                    volr = volr_t;
                    voll = (voll_t * (127 - pan)) / 63;
                }
                pan = _svm_pg[_svm_voice[i].prog].mpan;
                if (pan < 64) {
                    volr = (volr * pan) / 63;
                } else {
                    voll = (voll * (127 - pan)) / 63;
                }
                pan = _svm_voice[i].pan;
                if (pan < 64) {
                    volr = (volr * pan) / 63;
                } else {
                    voll = (voll * (127 - pan)) / 63;
                }
                if (_svm_stereo_mono == 1) {
                    if (voll < volr) {
                        voll = volr;
                    } else {
                        volr = voll;
                    }
                }
                voll = (voll * voll) / 0x3FFF;
                volr = (volr * volr) / 0x3FFF;
                ((short*)_svm_sreg_buf)[i * 8 + 0] = voll;
                ((short*)_svm_sreg_buf)[i * 8 + 1] = volr;
                _svm_sreg_dirty[i] |= 3;
            }
        }
    }
    return _svm_cur.seq_sep_no;
}
#else
// libsnd 3.3's SpuVmSetSeqVol: a volume of 0 stays 0, and mode 1 sets the
// sequence's voices to the volume as it is, without the tone, program and
// pan levels 4.0 works in.
short _SsVmSetSeqVol(short seq_sep_no, u16 voll, u16 volr, short arg3) {
    struct SeqStruct* score =
        &_ss_score[seq_sep_no & 0xFF][((u16)seq_sep_no >> 8) & 0xFF];
    int i;

    _svm_cur.seq_sep_no = seq_sep_no;
    score->voll = voll;
    score->volr = volr;
    if (score->voll >= 0x80) {
        score->voll = 0x7F;
    }
    if (score->volr >= 0x80) {
        score->volr = 0x7F;
    }
    if (arg3 == 1) {
        for (i = 0; i < _SsVmMaxVoice; i++) {
            if (_svm_voice[i].seq_sep_no == seq_sep_no) {
                _svm_sreg_buf[i].volume.left = voll * 129;
                _svm_sreg_buf[i].volume.right = volr * 129;
                _svm_sreg_dirty[i] |= 3;
            }
        }
    }
    return _svm_cur.seq_sep_no;
}
#endif

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/vm_seq", _SsVmGetSeqVol);

INCLUDE_ASM("asm/nonmatchings/libsnd/vm_seq", _SsVmGetSeqLVol);

INCLUDE_ASM("asm/nonmatchings/libsnd/vm_seq", _SsVmGetSeqRVol);
#else
// Written from libsnd 3.3's SpuVmGetSeqVol, SpuVmGetSeqLVol and
// SpuVmGetSeqRVol. A sequence is named by its access number in the low byte
// and its sequence number in the high byte.

static struct SeqStruct* Score(short seq_sep_no) {
    return &_ss_score[seq_sep_no & 0xFF][((u16)seq_sep_no >> 8) & 0xFF];
}

void _SsVmGetSeqVol(short seq_sep_no, short* voll, short* volr) {
    struct SeqStruct* score = Score(seq_sep_no);

    _svm_cur.seq_sep_no = seq_sep_no;
    *voll = score->voll;
    *volr = score->volr;
}

short _SsVmGetSeqLVol(short seq_sep_no) {
    _svm_cur.seq_sep_no = seq_sep_no & 0xFF;
    return Score(seq_sep_no)->voll;
}

short _SsVmGetSeqRVol(short seq_sep_no) {
    _svm_cur.seq_sep_no = seq_sep_no;
    return Score(seq_sep_no)->volr;
}
#endif

void _SsVmSeqKeyOff(s16 seq_sep_num) {
    u8 i;

    for (i = 0; i < _SsVmMaxVoice; i++) {
        if (_svm_voice[i].seq_sep_no == seq_sep_num) {
            _svm_cur.voice = i;
            _SsVmKeyOffNow(0);
        }
    }
}
