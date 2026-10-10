// Precise geometry: the table of precise vertices by the address their SXY
// was stored to. See src/precise.h.

#include <psyz.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../precise.h"

const PreciseVertex* precise_draw_words = NULL;
PsyzGeometry precise_mode = PSYZ_GEOMETRY_CONSOLE;
int precise_fade = 0;

#ifdef PSYZ_PRECISE_GEOMETRY

// Direct-mapped by word address, so a packet buffer of up to 512 KiB fills
// it without two of its words sharing a slot. A slot lost to another address
// only costs that vertex its precision.
#define TABLE_BITS 17
#define TABLE_MASK ((1u << TABLE_BITS) - 1)

typedef struct {
    uintptr_t addr;
    uint32_t value; // the word as it was stored
    uint32_t frame; // Precise_NextFrame's count when it was stored
    PreciseVertex v;
} Entry;

static Entry* table;
static uint32_t frame = 1;

static inline Entry* slot(const void* addr) {
    return &table[((uintptr_t)addr >> 2) & TABLE_MASK];
}

static inline uint32_t word(const void* addr) {
    uint32_t w;
    memcpy(&w, addr, sizeof(w));
    return w;
}

void Precise_Put(const void* addr, const PreciseVertex* v) {
    Entry* e;
    if (!table) {
        return;
    }
    e = slot(addr);
    e->addr = (uintptr_t)addr;
    e->value = word(addr);
    e->frame = frame;
    if (v) {
        e->v = *v;
    } else {
        e->v.w = e->v.fade = 0.0f;
    }
}

int Precise_Get(const void* addr, PreciseVertex* out) {
    const Entry* e;
    if (table) {
        e = slot(addr);
        // a polygon is drawn the frame after the one that built it
        if (e->addr == (uintptr_t)addr && (e->v.w > 0.0f || e->v.fade > 0.0f) &&
            e->value == word(addr) && frame - e->frame <= 1) {
            *out = e->v;
            return 1;
        }
    }
    out->w = out->fade = 0.0f;
    return 0;
}

void Precise_Copy(const void* dst, const void* src) {
    PreciseVertex v;
    if (word(dst) == word(src) && Precise_Get(src, &v)) {
        Precise_Put(dst, &v);
    } else {
        Precise_Put(dst, NULL);
    }
}

void Precise_NextFrame(void) { frame++; }

int Psyz_VideoSetGeometry(PsyzGeometry geometry) {
    if (geometry != PSYZ_GEOMETRY_CONSOLE &&
        geometry != PSYZ_GEOMETRY_PRECISE &&
        geometry != PSYZ_GEOMETRY_PERSPECTIVE) {
        return -1;
    }
    if (geometry != PSYZ_GEOMETRY_CONSOLE && !table) {
        table = calloc(TABLE_MASK + 1, sizeof(Entry));
        if (!table) {
            return -1;
        }
    }
    precise_mode = geometry;
    return 0;
}

PsyzGeometry Psyz_VideoGetGeometry(void) { return precise_mode; }

int Precise_SetFade(int on) {
    if (on && !table) {
        table = calloc(TABLE_MASK + 1, sizeof(Entry));
        if (!table) {
            return -1;
        }
    }
    precise_fade = on;
    return 0;
}

#else // PSYZ_PRECISE_GEOMETRY

void Precise_Put(const void* addr, const PreciseVertex* v) {}
int Precise_Get(const void* addr, PreciseVertex* out) {
    out->w = out->fade = 0.0f;
    return 0;
}
void Precise_Copy(const void* dst, const void* src) {}
void Precise_NextFrame(void) {}

int Psyz_VideoSetGeometry(PsyzGeometry geometry) {
    return geometry == PSYZ_GEOMETRY_CONSOLE ? 0 : -1;
}

PsyzGeometry Psyz_VideoGetGeometry(void) { return PSYZ_GEOMETRY_CONSOLE; }

int Precise_SetFade(int on) { return on ? -1 : 0; }

#endif // PSYZ_PRECISE_GEOMETRY
