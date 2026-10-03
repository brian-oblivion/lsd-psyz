// libpress: the MDEC (motion decoder) in software, and the VLC (Huffman)
// decode of Sony's BS bitstreams (versions 1 and 2, as STR movies carry).
//
// DecDCTvlc turns a BS frame into the MDEC's run-level codes; DecDCTin hands
// them to the "MDEC"; DecDCTout decodes macroblocks into the caller's buffer.
// On the console the transfers run on DMA in the background; here each one
// completes before it returns, and DecDCTout's callback runs before
// DecDCTout returns. Games that DecDCTout again from that callback (the usual
// strip-by-strip loop) therefore recurse once per strip.
//
// References: psx-spx's "MDEC" chapter, for the run-level format, the
// quantization and the colour conversion; ISO/IEC 11172-2 (MPEG-1) table
// B.14, the AC coefficient codes BS shares.

#include <psyz.h>
#include <libpress.h>
#include <psyz/log.h>
#include <math.h>
#include <string.h>

// The MDEC command word DecDCTvlc writes ahead of the run-level codes:
// command 1 (decode), 15-bit output; the low 16 bits count the words that
// follow.
#define MDEC_CMD_DECODE_15BIT 0x38000000
#define MDEC_DEPTH_MASK 0x18000000
#define MDEC_DEPTH_24BIT 0x10000000
#define MDEC_SET_BIT15 0x02000000
#define RL_END_OF_BLOCK 0xFE00

// The default quantization table, in zig-zag order: MPEG-1's intra matrix
// with a DC step of 2, as DecDCTReset loads it for both luma and chroma.
static const u_char default_iq[64] = {
    2,  16, 16, 19, 16, 19, 22, 22, 22, 22, 22, 22, 26, 24, 26, 27,
    27, 27, 26, 26, 26, 26, 27, 27, 27, 29, 29, 29, 34, 34, 34, 29,
    29, 29, 27, 27, 29, 29, 32, 32, 34, 34, 37, 38, 37, 35, 35, 34,
    35, 38, 38, 40, 40, 40, 48, 48, 46, 46, 56, 56, 58, 69, 69, 83,
};

// zig-zag position -> row-major index in an 8x8 block
static const u_char zigzag[64] = {
    0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6,  7,  14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63,
};

static struct {
    u_char iq_y[64];
    u_char iq_c[64];
    const u_short* in;     // run-level codes left to decode
    const u_short* in_end; // one past the last
    u32 command;           // DecDCTin's command word, with its mode applied
    DecDCCb in_cb;
    DecDCCb out_cb;
    int initialized;
} mdec;

static float idct_basis[8][8]; // [x][u] = C(u)/2 * cos((2x+1)u pi/16)

static void init_tables(void) {
    if (mdec.initialized) {
        return;
    }
    for (int x = 0; x < 8; x++) {
        for (int u = 0; u < 8; u++) {
            float c = u == 0 ? (float)M_SQRT1_2 : 1.0f;
            idct_basis[x][u] =
                0.5f * c * cosf((float)((2 * x + 1) * u) * (float)M_PI / 16.0f);
        }
    }
    mdec.initialized = 1;
}

void DecDCTReset(int mode) {
    init_tables();
    memcpy(mdec.iq_y, default_iq, sizeof(mdec.iq_y));
    memcpy(mdec.iq_c, default_iq, sizeof(mdec.iq_c));
    mdec.in = mdec.in_end = NULL;
    if (mode == 0) {
        mdec.in_cb = NULL;
        mdec.out_cb = NULL;
    }
}

DECDCTENV* DecDCTGetEnv(DECDCTENV* env) {
    memcpy(env->iq_y, mdec.iq_y, sizeof(env->iq_y));
    memcpy(env->iq_c, mdec.iq_c, sizeof(env->iq_c));
    return env;
}

DECDCTENV* DecDCTPutEnv(DECDCTENV* env) {
    memcpy(mdec.iq_y, env->iq_y, sizeof(mdec.iq_y));
    memcpy(mdec.iq_c, env->iq_c, sizeof(mdec.iq_c));
    return env;
}

// mode bit 0: 24-bit output (else 15-bit); bit 1: set bit 15 of each 15-bit
// pixel (the GPU's mask bit).
void DecDCTin(u_long* buf, int mode) {
    const u32 header = ((const u32*)buf)[0];
    init_tables();
    mdec.command = header & ~MDEC_DEPTH_MASK;
    mdec.command |= (mode & 1) ? MDEC_DEPTH_24BIT : (MDEC_CMD_DECODE_15BIT & MDEC_DEPTH_MASK);
    if (mode & 2) {
        mdec.command |= MDEC_SET_BIT15;
    }
    // The codes follow the header word, two to a 32-bit word.
    mdec.in = (const u_short*)((const u32*)buf + 1);
    mdec.in_end = mdec.in + (header & 0xFFFF) * 2;
    if (mdec.in_cb) {
        mdec.in_cb();
    }
}

int DecDCTinSync(int mode) {
    (void)mode;
    return 0;
}

int DecDCToutSync(int mode) {
    (void)mode;
    return 0;
}

DecDCCb DecDCTinCallback(DecDCCb func) {
    DecDCCb prev = mdec.in_cb;
    mdec.in_cb = func;
    return prev;
}

DecDCCb DecDCToutCallback(DecDCCb func) {
    DecDCCb prev = mdec.out_cb;
    mdec.out_cb = func;
    return prev;
}

static int sign_extend10(int n) { return (n & 0x200) ? (n | ~0x3FF) : (n & 0x3FF); }

// One 8x8 block: run-level codes, dequantized, through the IDCT. Returns 0
// when the input ran out first.
static int decode_block(float out[64], const u_char* iq) {
    int coef[64];
    memset(coef, 0, sizeof(coef));
    // Padding (0xFE00 words) may sit between blocks.
    while (mdec.in < mdec.in_end && *mdec.in == RL_END_OF_BLOCK) {
        mdec.in++;
    }
    if (mdec.in >= mdec.in_end) {
        return 0;
    }
    int n = *mdec.in++;
    const int qscale = n >> 10;
    int k = 0;
    int val = sign_extend10(n) * iq[0];
    for (;;) {
        if (qscale == 0) {
            val = sign_extend10(n) * 2;
        }
        if (val < -0x400) {
            val = -0x400;
        } else if (val > 0x3FF) {
            val = 0x3FF;
        }
        coef[qscale ? zigzag[k] : k] = val;
        if (mdec.in >= mdec.in_end) {
            break;
        }
        n = *mdec.in++;
        if (n == RL_END_OF_BLOCK) {
            break;
        }
        k += (n >> 10) + 1;
        if (k > 63) {
            break;
        }
        val = (sign_extend10(n) * iq[k] * qscale + 4) / 8;
    }

    // Separable 2D IDCT: rows, then columns.
    float tmp[64];
    for (int v = 0; v < 8; v++) {
        for (int x = 0; x < 8; x++) {
            float s = 0.0f;
            for (int u = 0; u < 8; u++) {
                s += idct_basis[x][u] * (float)coef[v * 8 + u];
            }
            tmp[v * 8 + x] = s;
        }
    }
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            float s = 0.0f;
            for (int v = 0; v < 8; v++) {
                s += idct_basis[y][v] * tmp[v * 8 + x];
            }
            out[y * 8 + x] = s;
        }
    }
    return 1;
}

// 15-bit output rounds each component to 5 bits; truncating left movies a
// half step (~4/255) darker than on the console.
static u_short to5(int v) { return v >= 0xF8 ? 31 : (v + 4) >> 3; }

static int clamp8(float v) {
    int i = (int)lrintf(v);
    return i < 0 ? 0 : i > 255 ? 255 : i;
}

// One 16x16 macroblock (Cr, Cb, then the four Y blocks) as 15-bit pixels
// (256 halfwords) or 24-bit ones (768 bytes), row by row.
static int decode_macroblock(void* dst) {
    float cr[64], cb[64], y[4][64];
    if (!decode_block(cr, mdec.iq_c) || !decode_block(cb, mdec.iq_c)) {
        return 0;
    }
    for (int i = 0; i < 4; i++) {
        if (!decode_block(y[i], mdec.iq_y)) {
            return 0;
        }
    }
    const int depth24 = (mdec.command & MDEC_DEPTH_MASK) == MDEC_DEPTH_24BIT;
    const u_short bit15 = (mdec.command & MDEC_SET_BIT15) ? 0x8000 : 0;
    for (int py = 0; py < 16; py++) {
        for (int px = 0; px < 16; px++) {
            const int c = (py >> 1) * 8 + (px >> 1);
            const float l =
                y[(py >> 3) * 2 + (px >> 3)][(py & 7) * 8 + (px & 7)] + 128.0f;
            const int r = clamp8(l + 1.402f * cr[c]);
            const int g = clamp8(l - 0.3437f * cb[c] - 0.7143f * cr[c]);
            const int b = clamp8(l + 1.772f * cb[c]);
            if (depth24) {
                u_char* p = (u_char*)dst + (py * 16 + px) * 3;
                p[0] = r;
                p[1] = g;
                p[2] = b;
            } else {
                ((u_short*)dst)[py * 16 + px] =
                    bit15 | to5(r) | (to5(g) << 5) | (to5(b) << 10);
            }
        }
    }
    return 1;
}

// size: the 32-bit words to produce, a whole number of macroblocks (128
// words each at 15 bits, 192 at 24).
void DecDCTout(u_long* buf, int size) {
    const int depth24 = (mdec.command & MDEC_DEPTH_MASK) == MDEC_DEPTH_24BIT;
    const int mb_words = depth24 ? 192 : 128;
    u32* out = (u32*)buf;
    for (int done = 0; done + mb_words <= size; done += mb_words) {
        if (!decode_macroblock(out + done)) {
            memset(out + done, 0, (size - done) * sizeof(u32));
            break;
        }
    }
    if (mdec.out_cb) {
        mdec.out_cb();
    }
}

// The VLC decode. BS streams are read as little-endian 16-bit words, each
// from its top bit down.
typedef struct {
    const u_short* p;
    const u_short* end;
    u32 bits;  // the next bits, MSB first
    int count;   // how many of `bits` are valid
} BitReader;

static void br_fill(BitReader* br) {
    while (br->count <= 16) {
        u32 w = br->p < br->end ? *br->p++ : 0;
        br->bits |= w << (16 - br->count);
        br->count += 16;
    }
}

static u32 br_peek(BitReader* br, int n) {
    br_fill(br);
    return br->bits >> (32 - n);
}

static void br_skip(BitReader* br, int n) {
    br->bits <<= n;
    br->count -= n;
}

static u32 br_read(BitReader* br, int n) {
    u32 v = br_peek(br, n);
    br_skip(br, n);
    return v;
}

// MPEG-1 table B.14 without its first-coefficient "1s" code: the code (sign
// bit not included), its length, run and level.
typedef struct {
    u_short code;
    u_char len;
    u_char run;
    u_char level;
} AcCode;

static const AcCode ac_codes[] = {
    {0x3, 2, 0, 1},     {0x3, 3, 1, 1},     {0x4, 4, 0, 2},
    {0x5, 4, 2, 1},     {0x5, 5, 0, 3},     {0x7, 5, 3, 1},
    {0x6, 5, 4, 1},     {0x6, 6, 1, 2},     {0x7, 6, 5, 1},
    {0x5, 6, 6, 1},     {0x4, 6, 7, 1},     {0x6, 7, 0, 4},
    {0x4, 7, 2, 2},     {0x7, 7, 8, 1},     {0x5, 7, 9, 1},
    {0x26, 8, 0, 5},    {0x21, 8, 0, 6},    {0x25, 8, 1, 3},
    {0x24, 8, 3, 2},    {0x27, 8, 10, 1},   {0x23, 8, 11, 1},
    {0x22, 8, 12, 1},   {0x20, 8, 13, 1},   {0xA, 10, 0, 7},
    {0xC, 10, 1, 4},    {0xB, 10, 2, 3},    {0xF, 10, 4, 2},
    {0x9, 10, 5, 2},    {0xE, 10, 14, 1},   {0xD, 10, 15, 1},
    {0x8, 10, 16, 1},   {0x1D, 12, 0, 8},   {0x18, 12, 0, 9},
    {0x13, 12, 0, 10},  {0x10, 12, 0, 11},  {0x1B, 12, 1, 5},
    {0x14, 12, 2, 4},   {0x1C, 12, 3, 3},   {0x12, 12, 4, 3},
    {0x1E, 12, 6, 2},   {0x15, 12, 7, 2},   {0x11, 12, 8, 2},
    {0x1F, 12, 17, 1},  {0x1A, 12, 18, 1},  {0x19, 12, 19, 1},
    {0x17, 12, 20, 1},  {0x16, 12, 21, 1},  {0x1A, 13, 0, 12},
    {0x19, 13, 0, 13},  {0x18, 13, 0, 14},  {0x17, 13, 0, 15},
    {0x16, 13, 1, 6},   {0x15, 13, 1, 7},   {0x14, 13, 2, 5},
    {0x13, 13, 3, 4},   {0x12, 13, 5, 3},   {0x11, 13, 9, 2},
    {0x10, 13, 10, 2},  {0x1F, 13, 22, 1},  {0x1E, 13, 23, 1},
    {0x1D, 13, 24, 1},  {0x1C, 13, 25, 1},  {0x1B, 13, 26, 1},
    {0x1F, 14, 0, 16},  {0x1E, 14, 0, 17},  {0x1D, 14, 0, 18},
    {0x1C, 14, 0, 19},  {0x1B, 14, 0, 20},  {0x1A, 14, 0, 21},
    {0x19, 14, 0, 22},  {0x18, 14, 0, 23},  {0x17, 14, 0, 24},
    {0x16, 14, 0, 25},  {0x15, 14, 0, 26},  {0x14, 14, 0, 27},
    {0x13, 14, 0, 28},  {0x12, 14, 0, 29},  {0x11, 14, 0, 30},
    {0x10, 14, 0, 31},  {0x18, 15, 0, 32},  {0x17, 15, 0, 33},
    {0x16, 15, 0, 34},  {0x15, 15, 0, 35},  {0x14, 15, 0, 36},
    {0x13, 15, 0, 37},  {0x12, 15, 0, 38},  {0x11, 15, 0, 39},
    {0x10, 15, 0, 40},  {0x1F, 15, 1, 8},   {0x1E, 15, 1, 9},
    {0x1D, 15, 1, 10},  {0x1C, 15, 1, 11},  {0x1B, 15, 1, 12},
    {0x1A, 15, 1, 13},  {0x19, 15, 1, 14},  {0x13, 16, 1, 15},
    {0x12, 16, 1, 16},  {0x11, 16, 1, 17},  {0x10, 16, 1, 18},
    {0x14, 16, 6, 3},   {0x1A, 16, 11, 2},  {0x19, 16, 12, 2},
    {0x18, 16, 13, 2},  {0x17, 16, 14, 2},  {0x16, 16, 15, 2},
    {0x15, 16, 16, 2},  {0x1F, 16, 27, 1},  {0x1E, 16, 28, 1},
    {0x1D, 16, 29, 1},  {0x1C, 16, 30, 1},  {0x1B, 16, 31, 1},
};

// Indexed by the next 17 bits would be 256 KB; two levels keep it small:
// codes up to 8 bits (plus sign) by their top 8 bits, longer ones (which
// all start with 0000 00) by the 10 bits after those six zeros.
typedef struct {
    u_char len; // code length without the sign bit; 0 = invalid
    u_char run;
    u_short level;
} AcEntry;

#define AC_EOB 0xFF
#define AC_ESCAPE 0xFE
static AcEntry ac_short[256];
static AcEntry ac_long[1024];
static int ac_tables_built;

static void build_ac_tables(void) {
    if (ac_tables_built) {
        return;
    }
    for (size_t i = 0; i < sizeof(ac_codes) / sizeof(*ac_codes); i++) {
        const AcCode* c = &ac_codes[i];
        AcEntry e = {c->len, c->run, c->level};
        if (c->len <= 8) {
            int shift = 8 - c->len;
            for (int j = 0; j < (1 << shift); j++) {
                ac_short[(c->code << shift) | j] = e;
            }
        } else {
            int shift = 16 - c->len; // within the 10 bits after 000000
            for (int j = 0; j < (1 << shift); j++) {
                ac_long[(c->code << shift) | j] = e;
            }
        }
    }
    // "10": end of block. "000001": escape, 6-bit run and 10-bit level.
    for (int j = 0; j < 64; j++) {
        ac_short[0x80 | j] = (AcEntry){2, AC_EOB, 0};
    }
    for (int j = 0; j < 4; j++) {
        ac_short[0x04 | j] = (AcEntry){6, AC_ESCAPE, 0};
    }
    ac_tables_built = 1;
}

static int bs_size_words(const u_short* bs) { return bs[0]; }

int DecDCTBufSize(u_long* bs) {
    // The MDEC command word, then the run-level codes.
    return bs_size_words((const u_short*)bs) + 1;
}

// Decodes a whole BS frame (header: run-level size in words, 0x3800, the
// quantization scale, the version) into `buf`. Returns 0: everything was
// decoded in one call. Versions 1 and 2 only; version 3 (DC prediction)
// is later than the Psy-Q releases most games link.
int DecDCTvlc(u_long* bs, u_long* buf) {
    if (bs == NULL) {
        return 0; // a resumed decode; ours never stops part-way
    }
    build_ac_tables();
    const u_short* h = (const u_short*)bs;
    const int size = bs_size_words(h);
    const int qscale = h[2] & 0x3F;
    const int version = h[3];
    if (version < 1 || version > 2) {
        LOG_ONCE("DecDCTvlc: BS version %d is not supported", version);
        ((u32*)buf)[0] = MDEC_CMD_DECODE_15BIT;
        return 0;
    }
    BitReader br = {h + 4, h + 4 + size * 2 + 2, 0, 0};
    u_short* out = (u_short*)((u32*)buf + 1);
    u_short* const out_end = out + size * 2;
    while (out < out_end) {
        // DC: 10 bits as they are
        *out++ = (u_short)((qscale << 10) | br_read(&br, 10));
        // AC, up to the end of the block
        while (out < out_end) {
            u32 top = br_peek(&br, 16);
            AcEntry e = (top >> 10) ? ac_short[top >> 8] : ac_long[top & 0x3FF];
            if (e.len == 0) {
                // Zeros: the stream's padding, after the last block.
                if (top != 0) {
                    LOG_ONCE("DecDCTvlc: invalid code %04X", top);
                }
                out = out_end;
                break;
            }
            if (e.run == AC_EOB) {
                br_skip(&br, 2);
                *out++ = RL_END_OF_BLOCK;
                break;
            }
            if (e.run == AC_ESCAPE) {
                br_skip(&br, 6);
                *out++ = (u_short)br_read(&br, 16);
                continue;
            }
            br_skip(&br, e.len);
            int level = e.level;
            if (br_read(&br, 1)) {
                level = -level;
            }
            *out++ = (u_short)((e.run << 10) | (level & 0x3FF));
        }
    }
    while (out < out_end) {
        *out++ = RL_END_OF_BLOCK;
    }
    ((u32*)buf)[0] = MDEC_CMD_DECODE_15BIT | size;
    return 0;
}
