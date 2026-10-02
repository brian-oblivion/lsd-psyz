#include <stddef.h>
#include <psyz.h>
#include <libgpu.h>
#include <libgte.h>
#include <libgs.h>
#include "ztest.h"

// TMD primitives are read straight from TMD files, so their sizes are the
// file format's: a 4-byte header and the packet's words.
ZTEST(libgs_types, tmd_primitive_sizes) {
    zexpect_u32_eq(16, sizeof(TMD_P_F3));
    zexpect_u32_eq(20, sizeof(TMD_P_G3));
    zexpect_u32_eq(24, sizeof(TMD_P_F3G));
    zexpect_u32_eq(28, sizeof(TMD_P_G3G));
    zexpect_u32_eq(16, sizeof(TMD_P_NF3));
    zexpect_u32_eq(24, sizeof(TMD_P_NG3));
    zexpect_u32_eq(20, sizeof(TMD_P_F4));
    zexpect_u32_eq(24, sizeof(TMD_P_G4));
    zexpect_u32_eq(16, sizeof(TMD_P_NF4));
    zexpect_u32_eq(28, sizeof(TMD_P_NG4));
    zexpect_u32_eq(24, sizeof(TMD_P_TF3));
    zexpect_u32_eq(28, sizeof(TMD_P_TG3));
    zexpect_u32_eq(28, sizeof(TMD_P_TNF3));
    zexpect_u32_eq(36, sizeof(TMD_P_TNG3));
    zexpect_u32_eq(32, sizeof(TMD_P_TF4));
    zexpect_u32_eq(36, sizeof(TMD_P_TG4));
    zexpect_u32_eq(32, sizeof(TMD_P_TNF4));
    zexpect_u32_eq(44, sizeof(TMD_P_TNG4));
}

ZTEST(libgs_types, tmd_textured_fields) {
    zexpect_u32_eq(6, offsetof(TMD_P_TF3, clut));
    zexpect_u32_eq(10, offsetof(TMD_P_TF3, tpage));
    zexpect_u32_eq(16, offsetof(TMD_P_TF3, n0));
    zexpect_u32_eq(18, offsetof(TMD_P_TF3, v0));
    zexpect_u32_eq(7, offsetof(TMD_P_F3, code));
}

// Games keep a flat light as three ints and a colour, and pass it cast.
ZTEST(libgs_types, flat_light_layout) {
    zexpect_u32_eq(0, offsetof(GsF_LIGHT, vx));
    zexpect_u32_eq(4, offsetof(GsF_LIGHT, vy));
    zexpect_u32_eq(8, offsetof(GsF_LIGHT, vz));
    zexpect_u32_eq(12, offsetof(GsF_LIGHT, r));
}

ZTEST(libgs_types, identity_matrix) {
    int i, j;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            zexpect_s16_eq(i == j ? 4096 : 0, GsIDMATRIX.m[i][j]);
        }
        zexpect_s32_eq(0, GsIDMATRIX.t[i]);
    }
}
