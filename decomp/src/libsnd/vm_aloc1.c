#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/vm_aloc1", _SsVmAlloc);
#else
// Picks the voice for the tone in _svm_cur: the last free voice, or else the
// busy voice of lowest priority (at most the tone's), the quietest envelope
// and then the oldest key-on breaking ties. Returns _SsVmMaxVoice when every
// busy voice outranks the tone. Written from libsnd 3.3's SpuVmAlloc.
char _SsVmAlloc(short voice) {
    int chosen = 99;
    int best = 99;
    int found = 0;
    unsigned short bestEnvx = 0xFFFF;
    int bestAge = 0;
    int threshold = (u8)_svm_cur.tone_prior;
    int i;

    for (i = 0; i < _SsVmMaxVoice; i++) {
        struct SpuVoice* v = &_svm_voice[i];
        if (v->unk1b == 0 && v->unk6 == 0) {
            chosen = i;
            continue;
        }
        if (v->unk18 < threshold) {
            threshold = v->unk18;
            best = i;
            bestEnvx = v->unk6;
            bestAge = v->unk2;
            found = 1;
        } else if (v->unk18 == threshold) {
            found++;
            if (v->unk6 < bestEnvx) {
                bestAge = v->unk2;
                bestEnvx = v->unk6;
                best = i;
            } else if (v->unk6 == bestEnvx && bestAge < (short)v->unk2) {
                bestAge = (short)v->unk2;
                best = i;
            }
        }
    }
    if (chosen == 99) {
        chosen = found ? best : _SsVmMaxVoice;
    }
    if (chosen < _SsVmMaxVoice) {
        for (i = 0; i < _SsVmMaxVoice; i++) {
            _svm_voice[i].unk2++;
        }
        _svm_voice[chosen].unk2 = 0;
        _svm_voice[chosen].unk18 = (u8)_svm_cur.tone_prior;
        if (_svm_voice[chosen].unk1b == 2) {
            SpuSetNoiseVoice(SPU_OFF, SPU_ALLCH);
        }
    }
    return chosen;
}
#endif
