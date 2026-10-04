#include <string.h>
#include <psyz.h>
#include <libetc.h>
#include <libgpu.h>
#include <libgte.h>
#include <libgs.h>
#include "ztest.h"

enum {
    GS_OT_LENGTH = 1,
    GS_SCREEN_WIDTH = 256,
    GS_SCREEN_HEIGHT = 240,
};

static GsOT gs_ot[2];
static GsOT_TAG gs_ot_tag[2][1 << GS_OT_LENGTH];
static PACKET gs_packets[2][64];
static TILE gs_tile;

// 15-bit VRAM colour of an 8-bit RGB triple
static u_short gs_rgb15(int r, int g, int b) {
    return (u_short)((r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10));
}

static u_short gs_read_pixel(int x, int y) {
    RECT rect = {(short)x, (short)y, 1, 1};
    u_short pixel[2] = {0, 0};
    DrawSync(0);
    StoreImage(&rect, (u_long*)pixel);
    DrawSync(0);
    // colour only: whether drawing sets the mask bit (15) is not under test
    return pixel[0] & 0x7FFF;
}

// Clears both buffers to black, then sorts and draws one frame into the
// current draw buffer: a clear to (r, g, b) and, if drawTile, a 16x16 red
// tile at (8, 8) in screen coordinates.
static int gs_frame(int r, int g, int b, int drawTile) {
    RECT all = {0, 0, GS_SCREEN_WIDTH, 2 * GS_SCREEN_HEIGHT};
    ClearImage(&all, 0, 0, 0);
    int buf = GsGetActiveBuff();
    GsSetWorkBase(gs_packets[buf]);
    GsClearOt(0, 0, &gs_ot[buf]);
    GsSortClear((u_char)r, (u_char)g, (u_char)b, &gs_ot[buf]);
    if (drawTile) {
        setTile(&gs_tile);
        setRGB0(&gs_tile, 255, 0, 0);
        setXY0(&gs_tile, 8, 8);
        setWH(&gs_tile, 16, 16);
        AddPrim((OT_TYPE*)gs_ot[buf].tag, &gs_tile);
    }
    GsDrawOt(&gs_ot[buf]);
    DrawSync(0);
    return buf;
}

static void gs_setup(void) {
    Psyz_VideoSetDitheringMode(PSYZ_DITHER_OFF);
    Psyz_VideoSetInternalResolution(1);
    ResetGraph(0);
    GsInitGraph(GS_SCREEN_WIDTH, GS_SCREEN_HEIGHT, GsNONINTER | GsOFSGPU, 0, 0);
    GsDefDispBuff(0, 0, 0, GS_SCREEN_HEIGHT);
    for (int i = 0; i < 2; i++) {
        gs_ot[i].length = GS_OT_LENGTH;
        gs_ot[i].org = gs_ot_tag[i];
        GsClearOt(0, 0, &gs_ot[i]);
    }
}

ZTEST_SETUP(gs) { gs_setup(); }

// GsSortClear fills the whole current draw buffer, whichever it is.
ZTEST(gs, sort_clear_fills_draw_buffer) {
    u_short grey = gs_rgb15(48, 48, 48);
    zexpect_s32_eq(0, gs_frame(48, 48, 48, 0));
    zprintf("buffer 0 (y 0..239) is cleared\n");
    zexpect_u16_eq(grey, gs_read_pixel(0, 0));
    zexpect_u16_eq(grey, gs_read_pixel(GS_SCREEN_WIDTH - 1, 239));
    zexpect_u16_eq(0, gs_read_pixel(0, 240));

    GsSwapDispBuff();
    zexpect_s32_eq(1, gs_frame(48, 48, 48, 0));
    zprintf("buffer 1 (y 240..479) is cleared\n");
    zexpect_u16_eq(grey, gs_read_pixel(0, 240));
    zexpect_u16_eq(grey, gs_read_pixel(GS_SCREEN_WIDTH - 1, 479));
    zexpect_u16_eq(0, gs_read_pixel(0, 239));
}

// With GsOFSGPU, primitives in screen coordinates land in the current draw
// buffer: GsDefDispBuff's second buffer starts at (x1, y1).
ZTEST(gs, draw_offset_follows_buffer) {
    u_short red = gs_rgb15(255, 0, 0);
    zexpect_s32_eq(0, gs_frame(0, 0, 0, 1));
    zprintf("buffer 0: the tile is at (8, 8)\n");
    zexpect_u16_eq(red, gs_read_pixel(8, 8));
    zexpect_u16_eq(red, gs_read_pixel(23, 23));
    zexpect_u16_eq(0, gs_read_pixel(8, 248));

    GsSwapDispBuff();
    zexpect_s32_eq(1, gs_frame(0, 0, 0, 1));
    zprintf("buffer 1: the tile is at (8, 248)\n");
    zexpect_u16_eq(red, gs_read_pixel(8, 248));
    zexpect_u16_eq(red, gs_read_pixel(23, 263));
    zexpect_u16_eq(0, gs_read_pixel(8, 8));
}

// 320x240: GsIDMATRIX2 is the identity, no aspect correction
ZTEST_SETUP(gs3d) {
    ResetGraph(0);
    GsInitGraph(320, 240, GsNONINTER | GsOFSGPU, 0, 0);
}

// 4096 * 1000 / |1000|, through SquareRoot0's table, as libgs computes it
static int gs_unit_1000(void) { return 4096000 / SquareRoot0(1000 * 1000); }

// A TMD's object table: offsets from the table become offsets from each
// object's entry, once (flag bit 0), and resolve with GsTMDAddr.
ZTEST(gs3d, map_modeling_data_relocates_once) {
    static u32 tmd[2 + 7 * 2];
    u8* table = (u8*)&tmd[2];
    memset(tmd, 0, sizeof(tmd));
    tmd[1] = 2;                  // two objects
    tmd[2 + 0] = 0x100;          // vertices
    tmd[2 + 2] = 0x200;          // normals
    tmd[2 + 4] = 0x300;          // primitives
    tmd[2 + 5] = 7;              // primitive count, untouched
    tmd[2 + 7 + 0] = 0x400;      // the second object's vertices
    GsMapModelingData((u_long*)tmd);
    zexpect_u32_eq(1, tmd[0]);
    zexpect_u32_eq(0x100, tmd[2 + 0]);
    zexpect_u32_eq(0x300, tmd[2 + 4]);
    zexpect_u32_eq(7, tmd[2 + 5]);
    zexpect_u32_eq(0x400 - 7 * 4, tmd[2 + 7 + 0]);
    zexpect_ptr_eq(table + 0x200, GsTMDAddr(&tmd[2], 2));
    zexpect_ptr_eq(table + 0x400, GsTMDAddr(&tmd[2 + 7], 0));
    GsMapModelingData((u_long*)tmd);
    zexpect_u32_eq(0x400 - 7 * 4, tmd[2 + 7 + 0]);
}

// Runs of one primitive mode: the first of each run gets the run's length
// in its first halfword. F3 is 0x10 bytes, GT4 0x24.
ZTEST(gs3d, link_object4_counts_mode_runs) {
    static struct {
        u32 obj[7 * 2];
        u32 prims[(0x10 * 3 + 0x24) / 4];
    } tmd;
    GsDOBJ2 dobj;
    u8* p = (u8*)tmd.prims;
    memset(&tmd, 0, sizeof(tmd));
    p[0x00 + 3] = 0x20;
    p[0x10 + 3] = 0x20;
    p[0x20 + 3] = 0x3C;
    p[0x44 + 3] = 0x20;
    tmd.obj[7 + 4] = (u32)((u8*)tmd.prims - (u8*)&tmd.obj[7]); // mapped
    tmd.obj[7 + 5] = 4;
    GsLinkObject4((u_long)tmd.obj, &dobj, 1);
    zexpect_ptr_eq(&tmd.obj[7], dobj.tmd);
    zexpect_u16_eq(2, *(u16*)(p + 0x00));
    zexpect_u16_eq(0, *(u16*)(p + 0x10));
    zexpect_u16_eq(1, *(u16*)(p + 0x20));
    zexpect_u16_eq(1, *(u16*)(p + 0x44));
}

// Viewpoint 1000 behind the origin on -z, looking at it: the world
// rotation is the identity, but for SquareRoot0's rounding, and the origin
// is 1000 in front.
static void gs_view_from_minus_z(int rz) {
    GsRVIEW2 view = {0};
    view.vpz = -1000;
    view.rz = rz;
    zassert_s32_eq(0, GsSetRefView2(&view));
}

ZTEST(gs3d, ref_view_from_minus_z) {
    const int k = gs_unit_1000();
    gs_view_from_minus_z(0);
    zexpect_s16_eq(k, GsWSMATRIX.m[0][0]);
    zexpect_s16_eq(4096, GsWSMATRIX.m[1][1]);
    zexpect_s16_eq(k, GsWSMATRIX.m[2][2]);
    zexpect_s16_eq(0, GsWSMATRIX.m[0][2]);
    zexpect_s32_eq(0, GsWSMATRIX.t[0]);
    zexpect_s32_eq(0, GsWSMATRIX.t[1]);
    zexpect_s32_eq(1000 * k >> 12, GsWSMATRIX.t[2]);
}

// Looking down +x: world x becomes view z.
ZTEST(gs3d, ref_view_along_x) {
    GsRVIEW2 view = {0};
    view.vrx = 1000;
    zassert_s32_eq(0, GsSetRefView2(&view));
    zexpect_s16_eq(0, GsWSMATRIX.m[0][0]);
    zexpect_s16_eq(-gs_unit_1000(), GsWSMATRIX.m[0][2]);
    zexpect_s16_eq(4096, GsWSMATRIX.m[1][1]);
    zexpect_s16_eq(gs_unit_1000(), GsWSMATRIX.m[2][0]);
    zexpect_s16_eq(0, GsWSMATRIX.m[2][2]);
}

// rz, in 4096ths of a degree, twists the view about its z axis.
ZTEST(gs3d, ref_view_twist) {
    gs_view_from_minus_z(90 * 4096);
    zexpect_s16_eq(0, GsWSMATRIX.m[0][0]);
    zexpect_s16_eq(4096, GsWSMATRIX.m[0][1]);
    zexpect_s16_eq(-gs_unit_1000(), GsWSMATRIX.m[1][0]);
    zexpect_s16_eq(0, GsWSMATRIX.m[1][1]);
}

ZTEST(gs3d, ref_view_needs_two_points) {
    GsRVIEW2 view = {0};
    zexpect_s32_eq(1, GsSetRefView2(&view));
}

// A child's local-to-world matrix includes its parent's, and both cache it
// for the frame; a child marked changed (flg 0) is recomputed.
ZTEST(gs3d, coordinate_chain) {
    GsCOORDINATE2 root, child;
    MATRIX lw, ls;
    GsInitCoordinate2(NULL, &root);
    GsInitCoordinate2(&root, &child);
    zexpect_ptr_eq(&child, root.sub);
    zexpect_ptr_eq(&root, child.super);
    root.coord.t[0] = 100;
    child.coord.t[1] = 50;
    GsGetLw(&child, &lw);
    zexpect_s32_eq(100, lw.t[0]);
    zexpect_s32_eq(50, lw.t[1]);
    zexpect_s16_eq(4096, lw.m[1][1]);
    zexpect_u32_eq(root.flg, child.flg);
    zexpect_s32_ne(0, child.flg);
    zexpect_s32_eq(50, child.workm.t[1]);

    child.coord.t[1] = 60;
    GsGetLw(&child, &lw);
    zprintf("cached this frame: the change is not seen\n");
    zexpect_s32_eq(50, lw.t[1]);
    child.flg = 0;
    GsGetLw(&child, &lw);
    zexpect_s32_eq(60, lw.t[1]);

    gs_view_from_minus_z(0);
    child.flg = 0;
    GsGetLws(&child, &lw, &ls);
    zexpect_s32_eq(100 * gs_unit_1000() >> 12, ls.t[0]);
    zexpect_s32_eq(60, ls.t[1]);
    zexpect_s32_eq(GsWSMATRIX.t[2], ls.t[2]);
    zexpect_s32_eq(0, lw.t[2]);
    child.flg = 0;
    GsGetLs(&child, &ls);
    zexpect_s32_eq(GsWSMATRIX.t[2], ls.t[2]);
}

// Light 0 shining down +z: its row of GsLIGHTWSMATRIX points back at the
// light, its colour column is the colour over 255 in 4096ths.
ZTEST(gs3d, flat_light) {
    GsF_LIGHT light = {0, 0, 1000, 255, 128, 0};
    GsF_LIGHT none = {0, 0, 0, 255, 255, 255};
    zexpect_s32_eq(0, GsSetFlatLight(0, &light));
    zexpect_s16_eq(0, GsLIGHTWSMATRIX.m[0][0]);
    zexpect_s16_eq(-gs_unit_1000(), GsLIGHTWSMATRIX.m[0][2]);
    zexpect_s16_eq(0, GsLIGHTWSMATRIX.m[1][2]);
    zexpect_s32_eq(-1, GsSetFlatLight(1, &none));
    zexpect_s16_eq(0, GsLIGHTWSMATRIX.m[1][2]);
}

ZTEST(gs3d, light_mode_range) {
    GsSetLightMode(2);
    zexpect_s32_eq(2, GsLIGHT_MODE);
    GsSetLightMode(4);
    zexpect_s32_eq(2, GsLIGHT_MODE);
    GsSetLightMode(0);
}
