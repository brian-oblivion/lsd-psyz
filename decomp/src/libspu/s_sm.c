#include "libspu_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libspu/s_sm", SpuSetMute);
#else
// SPU_ON mutes the SPU's output (SPUCNT bit 14 clear), SPU_OFF unmutes it.
long SpuSetMute(long on_off) {
    switch (on_off) {
    case SPU_OFF:
        SPUW(spucnt, SPUR(spucnt) | 0x4000);
        break;
    case SPU_ON:
        SPUW(spucnt, SPUR(spucnt) & ~0x4000);
        break;
    }
    return on_off;
}
#endif
