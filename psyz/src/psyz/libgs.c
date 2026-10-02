#include <psyz.h>
#include <libgpu.h>
#include <libgte.h>
#include <libgs.h>
#include <libetc.h>
#include <psyz/log.h>

// GsSortClear's primitives: a block fill, as PSY-Q uses (SetBlockFill), so
// the clear ignores the drawing offset and clip
static BLK_FILL tile_bg_clear[2];
static int HWD0;
static int VWD0;
static short GsORGOFSX;
static short GsORGOFSY;
static short PSDIDX;
static short PSDGPU;
static RECT CLIP2;
static DVECTOR POSITION;
static short PSDCNT;
static short PSDOFSX[2];
static short PSDOFSY[2];
static short PSDBASEX[2];
static short PSDBASEY[2];
static DRAWENV GsDRAWENV = {0};
static DISPENV GsDISPENV = {0};

PACKET* GsOUT_PACKET_P;
int GsLIGHT_MODE;
long GsCLIP3near;
long GsCLIP3far;
MATRIX GsIDMATRIX = {{{4096, 0, 0}, {0, 4096, 0}, {0, 0, 4096}}, {0, 0, 0}};

void gpu_init(unsigned short x, unsigned short y, unsigned short intmode,
              unsigned short dith, unsigned short varmmode) {

    if (((intmode >> 4) & 3) == 3) {
        ResetGraph(3);
    } else {
        ResetGraph(0);
    }
    GsDRAWENV.dtd = dith;
    PutDrawEnv(&GsDRAWENV);
    GsDISPENV.disp.w = (short)x;
    GsDISPENV.disp.h = (short)y;
    if (GetVideoMode() == 1) { // check if PAL
        GsDISPENV.screen.y = 24;
        GsDISPENV.pad0 = 1;
    }
    GsDISPENV.isrgb24 = varmmode;
    GsDISPENV.isinter = intmode & 1;
    PSDGPU = (short)(intmode & 4);
    PutDispEnv(&GsDISPENV);
}

void gte_init() {
    InitGeom();
    SetFarColor(0, 0, 0);
    SetGeomOffset(0, 0);
    GsORGOFSY = 0;
    GsORGOFSX = 0;
}

void GsClearVcount(void) { NOT_IMPLEMENTED; }

long GsGetVcount() {
    NOT_IMPLEMENTED;
    return 1;
}

void GsInitVcount() { NOT_IMPLEMENTED; }

// PSY-Q's: the OT is cleared in reverse, so the last tag is drawn first and
// tag 0 last (pri 0 is the front); the work pointer, which GsSortClear and
// GsDrawOt use, is the last tag.
void GsClearOt(unsigned short offset, unsigned short point, GsOT* otp) {
    otp->offset = offset;
    otp->point = point;
    otp->tag = otp->org + (1 << otp->length) - 1;
    ClearOTagR((OT_TYPE*)otp->org, 1 << otp->length);
}

void GsInitGraph(unsigned short x, unsigned short y, unsigned short intmode,
                 unsigned short dith, unsigned short varmmode) {
    gpu_init(x, y, intmode, dith, varmmode);
    gte_init();
    PSDIDX = 0;

    HWD0 = x;
    VWD0 = y;
    // TODO other missing inits
    PSDBASEX[0] = 0;
    PSDBASEX[1] = 0;
    PSDBASEY[0] = 0;
    PSDBASEY[1] = 0;
    // TODO other missing inits
    POSITION.vx = 0;
    POSITION.vy = 0;
    CLIP2.x = 0;
    CLIP2.y = 0;
    CLIP2.w = (short)x;
    CLIP2.h = (short)y;
    setlen(&tile_bg_clear[0], 3);
    setcode(&tile_bg_clear[0], 0x02);
    setlen(&tile_bg_clear[1], 3);
    setcode(&tile_bg_clear[1], 0x02);
    PSDCNT = 1;
    // TODO other missing inits

    GsSetDrawBuffClip();
    GsSetDrawBuffOffset();
}
void GsDefDispBuff(unsigned short x0, unsigned short y0, unsigned short x1,
                   unsigned short y1) {
    PSDOFSX[0] = (short)x0;
    PSDOFSX[1] = (short)x1;
    PSDOFSY[0] = (short)y0;
    PSDOFSY[1] = (short)y1;
    if (PSDGPU) {
        PSDBASEX[0] = 0;
        PSDBASEX[1] = 0;
        PSDBASEY[0] = 0;
        PSDBASEY[1] = 0;
    } else {
        PSDBASEX[0] = (short)x0;
        PSDBASEX[1] = (short)x1;
        PSDBASEY[0] = (short)y0;
        PSDBASEY[1] = (short)y1;
    }
    GsSetDrawBuffClip();
    GsSetDrawBuffOffset();
}

int GsGetActiveBuff(void) { return PSDIDX; }

void GsSetWorkBase(PACKET* outpacketp) { GsOUT_PACKET_P = outpacketp; }

void GsSwapDispBuff(void) {
    GsDISPENV.disp.x = (short)PSDOFSX[PSDIDX];
    GsDISPENV.disp.y = (short)PSDOFSY[PSDIDX];
    PutDispEnv(&GsDISPENV);
    SetDispMask(1);
    if (!PSDCNT++) {
        PSDCNT = 1;
    }
    PSDIDX = (short)(PSDIDX == 0);
    GsSetDrawBuffClip();
    GsSetDrawBuffOffset();
}

void GsSortClear(unsigned char r, unsigned char g, unsigned char b, GsOT* ot) {
    tile_bg_clear[PSDIDX].r0 = r;
    tile_bg_clear[PSDIDX].g0 = g;
    tile_bg_clear[PSDIDX].b0 = b;
    tile_bg_clear[PSDIDX].x0 = (short)PSDOFSX[PSDIDX];
    tile_bg_clear[PSDIDX].y0 = (short)PSDOFSY[PSDIDX];
    if (GsDISPENV.isrgb24) {
        tile_bg_clear[PSDIDX].w = (short)((3 * HWD0) >> 1);
    } else {
        tile_bg_clear[PSDIDX].w = (short)HWD0;
    }
    tile_bg_clear[PSDIDX].h = (short)VWD0;
    AddPrim((OT_TYPE*)ot->tag, &tile_bg_clear[PSDIDX]);
}

void GsDrawOt(GsOT* ot) { DrawOTag((OT_TYPE*)ot->tag); }

void GsSetDrawBuffClip(void) {
    GsDRAWENV.clip.x = CLIP2.x + PSDOFSX[PSDIDX];
    GsDRAWENV.clip.y = CLIP2.y + PSDOFSY[PSDIDX];
    GsDRAWENV.clip.w = CLIP2.w;
    GsDRAWENV.clip.h = CLIP2.h;
    PutDrawEnv(&GsDRAWENV);
}

void GsSetDrawBuffOffset(void) {
    if (PSDGPU) {
        // GsOFSGPU: the GPU adds the draw buffer's offset to every vertex
        GsDRAWENV.ofs[0] = POSITION.vx + PSDOFSX[PSDIDX];
        GsDRAWENV.ofs[1] = POSITION.vy + PSDOFSY[PSDIDX];
        GsORGOFSX = 0;
        GsORGOFSY = 0;
        PutDrawEnv(&GsDRAWENV);
    } else {
        // GsOFSGTE: the offset goes into the GTE instead; PSY-Q reads the
        // other buffer's offset here
        short x = POSITION.vx + PSDOFSX[PSDIDX == 0];
        short y = POSITION.vy + PSDOFSY[PSDIDX == 0];
        SetGeomOffset(x, y);
        GsORGOFSX = x;
        GsORGOFSY = y;
    }
}

// The 3D origin, (POSITION), is the screen's centre from here on.
void GsInit3D(void) {
    POSITION.vx = HWD0 / 2;
    POSITION.vy = VWD0 / 2;
    GsSetDrawBuffOffset();
    GsCLIP3near = 10;
    GsLIGHT_MODE = 0;
    GsCLIP3far = 0x3FFF;
}

// Takes effect at the next GsSetDrawBuffOffset (GsSwapDispBuff calls it).
void GsSetOrign(long x, long y) {
    POSITION.vx = (short)x;
    POSITION.vy = (short)y;
}

// `im` points past the TIM's id word, at its flags.
void GsGetTimInfo(u_long* im, GsIMAGE* tim) {
    const u32* p = (const u32*)im;
    tim->pmode = p[0];
    p++;
    if (tim->pmode & 8) {
        // the CLUT block: its size in bytes (header included), x, y, w, h
        const u32* next = (const u32*)((const u8*)p + (p[0] & ~3u));
        const u_short* h = (const u_short*)(p + 1);
        tim->cx = h[0];
        tim->cy = h[1];
        tim->cw = h[2];
        tim->ch = h[3];
        tim->clut = (u_long*)(p + 3);
        p = next;
    }
    const u_short* h = (const u_short*)(p + 1);
    tim->px = h[0];
    tim->py = h[1];
    tim->pw = h[2];
    tim->ph = h[3];
    tim->pixel = (u_long*)(p + 3);
}

#define GS_ONE 4096 // 1.0 in 20.12, the scale that leaves a sprite as it is

// GsSPRITE/GsBG/GsBOXF attribute bits
#define GS_ATTR_DOFF 0x80000000      // not displayed
#define GS_ATTR_ALON 0x40000000      // semi-transparent
#define GS_ATTR_ABR(a) (((a) >> 28) & 3)
#define GS_ATTR_NOROT 0x08000000     // GsSortSprite/GsSortBg: no rotation or scale
#define GS_ATTR_FLIPX 0x00800000     // GsSortSprite
#define GS_ATTR_FLIPY 0x00400000
#define GS_ATTR_TPF(a) (((a) >> 24) & 3) // 4-bit, 8-bit or 15-bit texture
#define GS_ATTR_NOBRIGHT 0x00000040  // texture drawn as is (r, g, b ignored)

// The OT tag `pri` names, as PSY-Q counts it from the OT's offset.
static OT_TYPE* gs_ot_tag(GsOT* otp, int pri) {
    int i = pri - (int)otp->offset;
    int n = 1 << otp->length;
    if (i < 0) {
        i = 0;
    } else if (i >= n) {
        i = n - 1;
    }
    return (OT_TYPE*)&otp->org[i];
}

static void* gs_alloc(size_t size) {
    void* p = GsOUT_PACKET_P;
    GsOUT_PACKET_P = (PACKET*)((u8*)p + size);
    return p;
}

static u_short gs_tpage(u_long attr, u_short tpage) {
    return (u_short)((tpage & 0x1F) | (GS_ATTR_TPF(attr) << 7) |
                     (GS_ATTR_ABR(attr) << 5));
}

void GsSortBoxFill(GsBOXF* boxf, GsOT* otp, unsigned short pri) {
    if (boxf->attribute & GS_ATTR_DOFF) {
        return;
    }
    DR_TPAGE* mode = gs_alloc(sizeof(DR_TPAGE));
    TILE* tile = gs_alloc(sizeof(TILE));
    setDrawTPage(mode, 0, 1, gs_tpage(boxf->attribute, 0));
    setTile(tile);
    setSemiTrans(tile, (boxf->attribute & GS_ATTR_ALON) != 0);
    setRGB0(tile, boxf->r, boxf->g, boxf->b);
    setXY0(tile, boxf->x + GsORGOFSX, boxf->y + GsORGOFSY);
    setWH(tile, boxf->w, boxf->h);
    // the mode first: the last added is drawn first
    OT_TYPE* tag = gs_ot_tag(otp, pri);
    addPrim(tag, tile);
    addPrim(tag, mode);
}

static u_char gs_clamp_uv(int v) { return v < 0 ? 0 : v > 255 ? 255 : v; }

// (x, y) is where the sprite's pivot (mx, my) goes, from the origin.
// Unrotated, unscaled and unflipped (or with GS_ATTR_NOROT) it is a SPRT;
// otherwise a POLY_FT4 through the GTE, at the projection distance so
// that the perspective divide leaves it as it is.
void GsSortSprite(GsSPRITE* sp, GsOT* otp, unsigned short pri) {
    const u_long attr = sp->attribute;
    if ((attr & GS_ATTR_DOFF) || sp->w == 0 || sp->h == 0) {
        return;
    }
    const int plain = sp->scalex == GS_ONE && sp->scaley == GS_ONE &&
                      sp->rotate == 0 &&
                      !(attr & (GS_ATTR_FLIPX | GS_ATTR_FLIPY));
    OT_TYPE* tag = gs_ot_tag(otp, pri);
    if ((attr & GS_ATTR_NOROT) || plain) {
        DR_TPAGE* mode = gs_alloc(sizeof(DR_TPAGE));
        SPRT* spr = gs_alloc(sizeof(SPRT));
        setDrawTPage(mode, 0, 1, gs_tpage(attr, sp->tpage));
        setSprt(spr);
        setSemiTrans(spr, (attr & GS_ATTR_ALON) != 0);
        setShadeTex(spr, (attr & GS_ATTR_NOBRIGHT) != 0);
        setRGB0(spr, sp->r, sp->g, sp->b);
        setXY0(spr, sp->x + GsORGOFSX - sp->mx, sp->y + GsORGOFSY - sp->my);
        setUV0(spr, sp->u, sp->v);
        spr->clut = getClut(sp->cx, sp->cy);
        setWH(spr, sp->w, sp->h);
        addPrim(tag, spr);
        addPrim(tag, mode);
        return;
    }

    MATRIX m;
    if (sp->rotate != 0) {
        SVECTOR r = {0, 0, (short)(sp->rotate / 360)};
        RotMatrix(&r, &m);
    } else {
        m = GsIDMATRIX;
    }
    if (sp->scalex != GS_ONE || sp->scaley != GS_ONE) {
        VECTOR s = {sp->scalex, sp->scaley, 0};
        ScaleMatrix(&m, &s);
    }
    VECTOR t = {sp->x, sp->y, ReadGeomScreen()};
    TransMatrix(&m, &t);
    SetRotMatrix(&m);
    SetTransMatrix(&m);
    SVECTOR v[4] = {
        {(short)-sp->mx, (short)-sp->my, 0},
        {(short)(sp->w - sp->mx), (short)-sp->my, 0},
        {(short)-sp->mx, (short)(sp->h - sp->my), 0},
        {(short)(sp->w - sp->mx), (short)(sp->h - sp->my), 0},
    };
    POLY_FT4* poly = gs_alloc(sizeof(POLY_FT4));
    setPolyFT4(poly);
    int sxy[4], p, flag;
    RotTransPers4(&v[0], &v[1], &v[2], &v[3], &sxy[0], &sxy[1], &sxy[2],
                  &sxy[3], &p, &flag);
    setXY4(poly, (short)sxy[0], (short)(sxy[0] >> 16), (short)sxy[1],
           (short)(sxy[1] >> 16), (short)sxy[2], (short)(sxy[2] >> 16),
           (short)sxy[3], (short)(sxy[3] >> 16));
    const int u_lo = sp->u, u_hi = sp->u + sp->w - 1;
    const int v_lo = sp->v, v_hi = sp->v + sp->h - 1;
    const int u0 = (attr & GS_ATTR_FLIPX) ? u_hi : u_lo;
    const int u1 = (attr & GS_ATTR_FLIPX) ? u_lo : u_hi;
    const int v0 = (attr & GS_ATTR_FLIPY) ? v_hi : v_lo;
    const int v1 = (attr & GS_ATTR_FLIPY) ? v_lo : v_hi;
    setUV4(poly, u0, v0, u1, v0, u0, v1, u1, v1);
    setSemiTrans(poly, (attr & GS_ATTR_ALON) != 0);
    setShadeTex(poly, (attr & GS_ATTR_NOBRIGHT) != 0);
    setRGB0(poly, sp->r, sp->g, sp->b);
    poly->clut = getClut(sp->cx, sp->cy);
    poly->tpage = gs_tpage(attr, sp->tpage);
    addPrim(tag, poly);
}

// One BG cell's quad. (px, py): its top-left from the pivot; (x, y): the
// pivot on screen; flag: the cell's flips (bit 1: x, bit 0: y). With a
// matrix (rotation or scale), the corners go through it.
typedef struct {
    u_char code, r, g, b;
    u_short tpage, clut;
    u_char u, v;
    short x, y, w, h, px, py;
    u_short flag;
} GsBgCell;

static void gs_bg_cell(POLY_FT4* poly, const GsBgCell* c, MATRIX* m) {
    const short ox = c->x + GsORGOFSX;
    const short oy = c->y + GsORGOFSY;
    int u0, u1, v0, v1;
    setPolyFT4(poly);
    if (m) {
        // corners inclusive, as PSY-Q's _mk_xpndsp
        u0 = (c->flag & 2) ? c->u + c->w - 1 : c->u;
        u1 = (c->flag & 2) ? c->u : c->u + c->w - 1;
        v0 = (c->flag & 1) ? c->v + c->h - 1 : c->v;
        v1 = (c->flag & 1) ? c->v : c->v + c->h - 1;
        const short cx[4] = {c->px, c->px + c->w, c->px, c->px + c->w};
        const short cy[4] = {c->py, c->py, c->py + c->h, c->py + c->h};
        short* xy[4] = {&poly->x0, &poly->x1, &poly->x2, &poly->x3};
        for (int i = 0; i < 4; i++) {
            SVECTOR in = {cx[i], cy[i], 0};
            VECTOR out;
            ApplyMatrix(m, &in, &out);
            xy[i][0] = (short)(ox + out.vx);
            xy[i][1] = (short)(oy + out.vy);
        }
    } else {
        // edges exclusive, as PSY-Q's _mk_normsp, clamped to the page
        u0 = (c->flag & 2) ? c->u + c->w - 1 : c->u;
        u1 = (c->flag & 2) ? c->u - 1 : c->u + c->w;
        v0 = (c->flag & 1) ? c->v + c->h - 1 : c->v;
        v1 = (c->flag & 1) ? c->v - 1 : c->v + c->h;
        const short x0 = ox + c->px, y0 = oy + c->py;
        setXY4(poly, x0, y0, x0 + c->w, y0, x0, y0 + c->h, x0 + c->w,
               y0 + c->h);
    }
    setUV4(poly, gs_clamp_uv(u0), gs_clamp_uv(v0), gs_clamp_uv(u1),
           gs_clamp_uv(v0), gs_clamp_uv(u0), gs_clamp_uv(v1),
           gs_clamp_uv(u1), gs_clamp_uv(v1));
    setRGB0(poly, c->r, c->g, c->b);
    poly->code = c->code;
    poly->tpage = c->tpage;
    poly->clut = c->clut;
}

static int gs_mod(int a, int b) {
    int r = a % b;
    return r < 0 ? r + b : r;
}

// The w x h window of the map at (scrollx, scrolly), wrapping, with its
// pivot (mx, my) at (x, y) from the origin; one POLY_FT4 per visible cell
// (or part of one). Map index 0xFFFF is an empty cell.
void GsSortBg(GsBG* bg, GsOT* otp, unsigned short pri) {
    const u_long attr = bg->attribute;
    GsMAP* map = bg->map;
    if ((attr & GS_ATTR_DOFF) || map->ncellw == 0 || map->ncellh == 0) {
        return;
    }
    const int cw = map->cellw ? map->cellw : 256;
    const int ch = map->cellh ? map->cellh : 256;
    const int mapw = map->ncellw * cw;
    const int maph = map->ncellh * ch;

    MATRIX m;
    MATRIX* mp = NULL;
    if (!(attr & GS_ATTR_NOROT) &&
        (bg->scalex != GS_ONE || bg->scaley != GS_ONE || bg->rotate != 0)) {
        m = GsIDMATRIX;
        if (bg->rotate != 0) {
            int a = bg->rotate / 360;
            MATRIX rz = {{{(short)rcos(a), (short)-rsin(a), 0},
                          {(short)rsin(a), (short)rcos(a), 0},
                          {0, 0, GS_ONE}}};
            MulMatrix(&m, &rz);
        }
        for (int i = 0; i < 3; i++) {
            m.m[i][0] = (short)((m.m[i][0] * bg->scalex) >> 12);
            m.m[i][1] = (short)((m.m[i][1] * bg->scaley) >> 12);
        }
        mp = &m;
    }

    GsBgCell c;
    c.code = 0x2C | ((attr & GS_ATTR_ALON) ? 2 : 0) |
             ((attr & GS_ATTR_NOBRIGHT) ? 1 : 0);
    c.r = bg->r;
    c.g = bg->g;
    c.b = bg->b;
    c.x = bg->x;
    c.y = bg->y;

    OT_TYPE* tag = gs_ot_tag(otp, pri);
    const int sx = gs_mod(bg->scrollx, mapw);
    const int sy = gs_mod(bg->scrolly, maph);
    for (int y = 0; y < bg->h;) {
        const int my = gs_mod(sy + y, maph);
        const int offy = my % ch;
        int h = ch - offy;
        if (h > bg->h - y) {
            h = bg->h - y;
        }
        for (int x = 0; x < bg->w;) {
            const int mx = gs_mod(sx + x, mapw);
            const int offx = mx % cw;
            int w = cw - offx;
            if (w > bg->w - x) {
                w = bg->w - x;
            }
            const u_short index =
                map->index[(my / ch) * map->ncellw + mx / cw];
            if (index != 0xFFFF) {
                const GsCELL* cell = &map->base[index];
                c.tpage = gs_tpage(attr, cell->tpage);
                c.clut = cell->cba;
                c.flag = cell->flag;
                c.u = (cell->flag & 2) ? cell->u + cw - offx - w
                                       : cell->u + offx;
                c.v = (cell->flag & 1) ? cell->v + ch - offy - h
                                       : cell->v + offy;
                c.w = (short)w;
                c.h = (short)h;
                c.px = (short)(x - bg->mx);
                c.py = (short)(y - bg->my);
                POLY_FT4* poly = gs_alloc(sizeof(POLY_FT4));
                gs_bg_cell(poly, &c, mp);
                addPrim(tag, poly);
            }
            x += w;
        }
        y += h;
    }
}
