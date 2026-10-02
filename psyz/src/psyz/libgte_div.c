// Recursive polygon subdivision: RCpolyF3, RCpolyFT3, RCpolyG3, RCpolyGT3,
// RCpolyF4, RCpolyFT4, RCpolyG4 and RCpolyGT4.
//
// The caller fills a DIVPOLYGON3/4 with the clip window (pih, piv), the
// primitive's colour and code (rgbc), clut/tpage for textured primitives, the
// OT entry (ot), the subdivision depth (ndiv) and points cr[0].r0, r1, ...
// at RVECTORs that hold each corner's local vertex, screen XY, screen Z, UV
// and colour. Every level of the recursion:
//
// 1. Rejects the polygon if all of its corners are nearer than H / 2, or all
//    of them lie beyond the same edge of the window centred on the GTE
//    screen offset (OFX +/- pih / 2, OFY +/- piv / 2).
// 2. Averages the corners into the edge midpoints (and, for quads, the
//    centre) of the local vertex, UV and colour, with plain integer shifts,
//    and projects the new vertices with RTPT under the current rotation and
//    translation matrices.
// 3. On the last level (ndiv) emits four primitives into the packet buffer,
//    each linked at the head of the single OT entry; on any other level
//    recurses into the four smaller polygons using cr[level + 1].
//
// Corners keep the screen XY the caller gave them; only new vertices are
// projected. cr[] doubles as the recursion stack: cr[level] holds the corner
// pointers of the polygon being split and receives its midpoints.

#include <psyz.h>
#include <libgte.h>
#include <libgpu.h>
#include <psyz/gte.h>
#include <string.h>

enum {
    DIV_TEXTURED = 1,
    DIV_GOURAUD = 2,
};

#define DIV_MAX_LEVELS 5 // number of entries in DIVPOLYGON3/4.cr[]

typedef struct {
    unsigned int near_z; // H / 2: corners with sz below it are "too near"
    int left, right, top, bottom;
    int ndiv;
    int kind;
    u_char* s; // next free packet
} DivState;

static int div_setup(
    DivState* st, u_long ndiv, u_long pih, u_long piv, int kind, void* s) {
    // CFC2 sign-extends H, so the halving is arithmetic on the 16-bit value
    int h = (short)Psyz_GteCtrlRead(26);
    int ofx = (int)Psyz_GteCtrlRead(24) >> 16;
    int ofy = (int)Psyz_GteCtrlRead(25) >> 16;
    int hw = (int)((unsigned int)pih >> 1);
    int hh = (int)((unsigned int)piv >> 1);

    st->near_z = (unsigned int)(h >> 1);
    st->left = ofx - hw;
    st->right = ofx + hw;
    st->top = ofy - hh;
    st->bottom = ofy + hh;
    st->kind = kind;
    st->s = (u_char*)s;
    // ndiv 0 never terminates on hardware, and more levels than cr[] has
    // entries run off the end of the DIVPOLYGON; neither is usable.
    if (ndiv == 0) {
        return 0;
    }
    st->ndiv = ndiv > DIV_MAX_LEVELS ? DIV_MAX_LEVELS : (int)ndiv;
    return 1;
}

static int div_rejected(const DivState* st, RVECTOR* const* r, int n) {
    int i, all;

    for (all = 1, i = 0; i < n; i++) {
        all &= (unsigned int)r[i]->sz < st->near_z;
    }
    if (all) {
        return 1;
    }
    for (all = 1, i = 0; i < n; i++) {
        all &= r[i]->sxy.vx > st->right;
    }
    if (all) {
        return 1;
    }
    for (all = 1, i = 0; i < n; i++) {
        all &= r[i]->sxy.vx < st->left;
    }
    if (all) {
        return 1;
    }
    for (all = 1, i = 0; i < n; i++) {
        all &= r[i]->sxy.vy > st->bottom;
    }
    if (all) {
        return 1;
    }
    for (all = 1, i = 0; i < n; i++) {
        all &= r[i]->sxy.vy < st->top;
    }
    return all;
}

// Midpoint of a and b in every attribute the primitive type uses.
static void div_mid2(RVECTOR* o, const RVECTOR* a, const RVECTOR* b, int kind) {
    o->v.vx = (short)((a->v.vx + b->v.vx) >> 1);
    o->v.vy = (short)((a->v.vy + b->v.vy) >> 1);
    o->v.vz = (short)((a->v.vz + b->v.vz) >> 1);
    if (kind & DIV_TEXTURED) {
        o->uv[0] = (u_char)((a->uv[0] + b->uv[0]) >> 1);
        o->uv[1] = (u_char)((a->uv[1] + b->uv[1]) >> 1);
    }
    if (kind & DIV_GOURAUD) {
        o->c.r = (u_char)((a->c.r + b->c.r) >> 1);
        o->c.g = (u_char)((a->c.g + b->c.g) >> 1);
        o->c.b = (u_char)((a->c.b + b->c.b) >> 1);
    }
}

// Centre of a quad: the four-corner sum shifted once, not an average of
// midpoints, so it rounds differently from div_mid2(div_mid2(), div_mid2()).
static void div_mid4(RVECTOR* o, const RVECTOR* a, const RVECTOR* b,
                     const RVECTOR* c, const RVECTOR* d, int kind) {
    o->v.vx = (short)((a->v.vx + b->v.vx + c->v.vx + d->v.vx) >> 2);
    o->v.vy = (short)((a->v.vy + b->v.vy + c->v.vy + d->v.vy) >> 2);
    o->v.vz = (short)((a->v.vz + b->v.vz + c->v.vz + d->v.vz) >> 2);
    if (kind & DIV_TEXTURED) {
        o->uv[0] = (u_char)((a->uv[0] + b->uv[0] + c->uv[0] + d->uv[0]) >> 2);
        o->uv[1] = (u_char)((a->uv[1] + b->uv[1] + c->uv[1] + d->uv[1]) >> 2);
    }
    if (kind & DIV_GOURAUD) {
        o->c.r = (u_char)((a->c.r + b->c.r + c->c.r + d->c.r) >> 2);
        o->c.g = (u_char)((a->c.g + b->c.g + c->c.g + d->c.g) >> 2);
        o->c.b = (u_char)((a->c.b + b->c.b + c->c.b + d->c.b) >> 2);
    }
}

// Projects three vertices; sz is only written when wanted, as the corners of
// the last level are never clip-tested again.
static void div_project3(RVECTOR* a, RVECTOR* b, RVECTOR* c, int store_sz) {
    unsigned int sxy[3], sz[3];

    gte_ldv3(&a->v, &b->v, &c->v);
    gte_rtpt();
    gte_stsxy3(&sxy[0], &sxy[1], &sxy[2]);
    memcpy(&a->sxy, &sxy[0], sizeof(a->sxy));
    memcpy(&b->sxy, &sxy[1], sizeof(b->sxy));
    memcpy(&c->sxy, &sxy[2], sizeof(c->sxy));
    if (store_sz) {
        gte_stsz3(&sz[0], &sz[1], &sz[2]);
        a->sz = sz[0];
        b->sz = sz[1];
        c->sz = sz[2];
    }
}

static void div_link(u_long* ot, void* p, int len) {
    addPrim(ot, p);
    setlen(p, len);
}

#define DIV_XY(P, N, V) ((P)->x##N = (V)->sxy.vx, (P)->y##N = (V)->sxy.vy)
#define DIV_UV(P, N, V) ((P)->u##N = (V)->uv[0], (P)->v##N = (V)->uv[1])
#define DIV_RGB(P, N, V)                                                       \
    ((P)->r##N = (V)->c.r, (P)->g##N = (V)->c.g, (P)->b##N = (V)->c.b)

// The flat primitives take the whole rgbc word, code included. The Gouraud
// ones store the code into the first corner's colour (that RVECTOR keeps it)
// and copy each corner's colour word, cd byte into the pad.
static void div_flat_rgbc(void* p, const CVECTOR* rgbc) {
    P_TAG* t = (P_TAG*)p;
    t->r0 = rgbc->r;
    t->g0 = rgbc->g;
    t->b0 = rgbc->b;
    t->code = rgbc->cd;
}

static void div_emit3(
    DivState* st, DIVPOLYGON3* d, RVECTOR* a, RVECTOR* b, RVECTOR* c) {
    switch (st->kind) {
    case 0: {
        POLY_F3* p = (POLY_F3*)st->s;
        div_flat_rgbc(p, &d->rgbc);
        DIV_XY(p, 0, a);
        DIV_XY(p, 1, b);
        DIV_XY(p, 2, c);
        div_link(d->ot, p, 4);
        st->s += sizeof(POLY_F3);
        break;
    }
    case DIV_TEXTURED: {
        POLY_FT3* p = (POLY_FT3*)st->s;
        div_flat_rgbc(p, &d->rgbc);
        DIV_XY(p, 0, a);
        DIV_UV(p, 0, a);
        p->clut = d->clut;
        DIV_XY(p, 1, b);
        DIV_UV(p, 1, b);
        p->tpage = d->tpage;
        DIV_XY(p, 2, c);
        DIV_UV(p, 2, c);
        p->pad1 = 0;
        div_link(d->ot, p, 7);
        st->s += sizeof(POLY_FT3);
        break;
    }
    case DIV_GOURAUD: {
        POLY_G3* p = (POLY_G3*)st->s;
        a->c.cd = d->rgbc.cd;
        DIV_RGB(p, 0, a);
        p->code = a->c.cd;
        DIV_XY(p, 0, a);
        DIV_RGB(p, 1, b);
        p->pad1 = b->c.cd;
        DIV_XY(p, 1, b);
        DIV_RGB(p, 2, c);
        p->pad2 = c->c.cd;
        DIV_XY(p, 2, c);
        div_link(d->ot, p, 6);
        st->s += sizeof(POLY_G3);
        break;
    }
    default: {
        POLY_GT3* p = (POLY_GT3*)st->s;
        a->c.cd = d->rgbc.cd;
        DIV_RGB(p, 0, a);
        p->code = a->c.cd;
        DIV_XY(p, 0, a);
        DIV_UV(p, 0, a);
        p->clut = d->clut;
        DIV_RGB(p, 1, b);
        p->p1 = b->c.cd;
        DIV_XY(p, 1, b);
        DIV_UV(p, 1, b);
        p->tpage = d->tpage;
        DIV_RGB(p, 2, c);
        p->p2 = c->c.cd;
        DIV_XY(p, 2, c);
        DIV_UV(p, 2, c);
        p->pad2 = 0;
        div_link(d->ot, p, 9);
        st->s += sizeof(POLY_GT3);
        break;
    }
    }
}

static void div_emit4(DivState* st, DIVPOLYGON4* d, RVECTOR* a, RVECTOR* b,
                      RVECTOR* c, RVECTOR* e) {
    switch (st->kind) {
    case 0: {
        POLY_F4* p = (POLY_F4*)st->s;
        div_flat_rgbc(p, &d->rgbc);
        DIV_XY(p, 0, a);
        DIV_XY(p, 1, b);
        DIV_XY(p, 2, c);
        DIV_XY(p, 3, e);
        div_link(d->ot, p, 5);
        st->s += sizeof(POLY_F4);
        break;
    }
    case DIV_TEXTURED: {
        POLY_FT4* p = (POLY_FT4*)st->s;
        div_flat_rgbc(p, &d->rgbc);
        DIV_XY(p, 0, a);
        DIV_UV(p, 0, a);
        p->clut = d->clut;
        DIV_XY(p, 1, b);
        DIV_UV(p, 1, b);
        p->tpage = d->tpage;
        DIV_XY(p, 2, c);
        DIV_UV(p, 2, c);
        p->pad1 = 0;
        DIV_XY(p, 3, e);
        DIV_UV(p, 3, e);
        p->pad2 = 0;
        div_link(d->ot, p, 9);
        st->s += sizeof(POLY_FT4);
        break;
    }
    case DIV_GOURAUD: {
        POLY_G4* p = (POLY_G4*)st->s;
        a->c.cd = d->rgbc.cd;
        DIV_RGB(p, 0, a);
        p->code = a->c.cd;
        DIV_XY(p, 0, a);
        DIV_RGB(p, 1, b);
        p->pad1 = b->c.cd;
        DIV_XY(p, 1, b);
        DIV_RGB(p, 2, c);
        p->pad2 = c->c.cd;
        DIV_XY(p, 2, c);
        DIV_RGB(p, 3, e);
        p->pad3 = e->c.cd;
        DIV_XY(p, 3, e);
        div_link(d->ot, p, 8);
        st->s += sizeof(POLY_G4);
        break;
    }
    default: {
        POLY_GT4* p = (POLY_GT4*)st->s;
        a->c.cd = d->rgbc.cd;
        DIV_RGB(p, 0, a);
        p->code = a->c.cd;
        DIV_XY(p, 0, a);
        DIV_UV(p, 0, a);
        p->clut = d->clut;
        DIV_RGB(p, 1, b);
        p->p1 = b->c.cd;
        DIV_XY(p, 1, b);
        DIV_UV(p, 1, b);
        p->tpage = d->tpage;
        DIV_RGB(p, 2, c);
        p->p2 = c->c.cd;
        DIV_XY(p, 2, c);
        DIV_UV(p, 2, c);
        p->pad2 = 0;
        DIV_RGB(p, 3, e);
        p->p3 = e->c.cd;
        DIV_XY(p, 3, e);
        DIV_UV(p, 3, e);
        p->pad3 = 0;
        div_link(d->ot, p, 12);
        st->s += sizeof(POLY_GT4);
        break;
    }
    }
}

// Triangle r0, r1, r2 split at r01, r12, r20:
//
//          r0
//         /  \
//      r01 -- r20
//      / \    / \
//    r1 - r12 -- r2
static void div_tri(DivState* st, DIVPOLYGON3* d, int level) {
    CRVECTOR3* cr = &d->cr[level];
    RVECTOR* corners[3];
    int leaf;

    corners[0] = cr->r0;
    corners[1] = cr->r1;
    corners[2] = cr->r2;
    if (div_rejected(st, corners, 3)) {
        return;
    }
    div_mid2(&cr->r01, cr->r0, cr->r1, st->kind);
    div_mid2(&cr->r12, cr->r1, cr->r2, st->kind);
    div_mid2(&cr->r20, cr->r2, cr->r0, st->kind);
    leaf = level + 1 == st->ndiv;
    div_project3(&cr->r01, &cr->r12, &cr->r20, !leaf);

    if (leaf) {
        div_emit3(st, d, cr->r1, &cr->r12, &cr->r01);
        div_emit3(st, d, &cr->r01, &cr->r12, &cr->r20);
        div_emit3(st, d, cr->r0, &cr->r01, &cr->r20);
        div_emit3(st, d, cr->r2, &cr->r20, &cr->r12);
    } else {
        CRVECTOR3* next = &d->cr[level + 1];
        next->r0 = cr->r0;
        next->r1 = &cr->r01;
        next->r2 = &cr->r20;
        div_tri(st, d, level + 1);
        next->r0 = cr->r1;
        next->r1 = &cr->r12;
        next->r2 = &cr->r01;
        div_tri(st, d, level + 1);
        next->r0 = cr->r2;
        next->r1 = &cr->r20;
        next->r2 = &cr->r12;
        div_tri(st, d, level + 1);
        next->r0 = &cr->r01;
        next->r1 = &cr->r12;
        next->r2 = &cr->r20;
        div_tri(st, d, level + 1);
    }
}

// Quad r0, r1, r2, r3 (GPU order: r3 is opposite r0) split at the edge
// midpoints r01, r02, r31, r32 and the centre rc:
//
//    r0 -- r01 -- r1
//    |      |      |
//   r02 --  rc -- r31
//    |      |      |
//    r2 -- r32 -- r3
static void div_quad(DivState* st, DIVPOLYGON4* d, int level) {
    CRVECTOR4* cr = &d->cr[level];
    RVECTOR* corners[4];
    int leaf;

    corners[0] = cr->r0;
    corners[1] = cr->r1;
    corners[2] = cr->r2;
    corners[3] = cr->r3;
    if (div_rejected(st, corners, 4)) {
        return;
    }
    leaf = level + 1 == st->ndiv;
    div_mid2(&cr->r01, cr->r0, cr->r1, st->kind);
    div_mid2(&cr->r02, cr->r0, cr->r2, st->kind);
    div_mid2(&cr->r31, cr->r3, cr->r1, st->kind);
    div_mid2(&cr->r32, cr->r3, cr->r2, st->kind);
    if (st->kind == 0) {
        // RCpolyF4 takes the centre as the midpoint of r01 and r32
        div_mid2(&cr->rc, &cr->r01, &cr->r32, 0);
    } else {
        div_mid4(&cr->rc, cr->r0, cr->r1, cr->r3, cr->r2, st->kind);
    }
    div_project3(&cr->r01, &cr->r02, &cr->rc, 1);
    div_project3(&cr->r31, &cr->r32, &cr->rc, !leaf);

    if (leaf) {
        div_emit4(st, d, cr->r0, &cr->r01, &cr->r02, &cr->rc);
        div_emit4(st, d, cr->r1, &cr->r31, &cr->r01, &cr->rc);
        div_emit4(st, d, cr->r2, &cr->r02, &cr->r32, &cr->rc);
        div_emit4(st, d, cr->r3, &cr->r32, &cr->r31, &cr->rc);
    } else {
        CRVECTOR4* next = &d->cr[level + 1];
        next->r0 = cr->r0;
        next->r1 = &cr->r01;
        next->r2 = &cr->r02;
        next->r3 = &cr->rc;
        div_quad(st, d, level + 1);
        next->r0 = cr->r1;
        next->r1 = &cr->r31;
        next->r2 = &cr->r01;
        next->r3 = &cr->rc;
        div_quad(st, d, level + 1);
        next->r0 = cr->r2;
        next->r1 = &cr->r02;
        next->r2 = &cr->r32;
        next->r3 = &cr->rc;
        div_quad(st, d, level + 1);
        next->r0 = cr->r3;
        next->r1 = &cr->r32;
        next->r2 = &cr->r31;
        next->r3 = &cr->rc;
        div_quad(st, d, level + 1);
    }
}

static u_long* div3(void* s, DIVPOLYGON3* divp, int kind) {
    DivState st;
    if (!div_setup(&st, divp->ndiv, divp->pih, divp->piv, kind, s)) {
        return (u_long*)s;
    }
    div_tri(&st, divp, 0);
    return (u_long*)st.s;
}

static u_long* div4(void* s, DIVPOLYGON4* divp, int kind) {
    DivState st;
    if (!div_setup(&st, divp->ndiv, divp->pih, divp->piv, kind, s)) {
        return (u_long*)s;
    }
    div_quad(&st, divp, 0);
    return (u_long*)st.s;
}

u_long* RCpolyF3(void* s, DIVPOLYGON3* divp) { return div3(s, divp, 0); }
u_long* RCpolyFT3(void* s, DIVPOLYGON3* divp) {
    return div3(s, divp, DIV_TEXTURED);
}
u_long* RCpolyG3(void* s, DIVPOLYGON3* divp) {
    return div3(s, divp, DIV_GOURAUD);
}
u_long* RCpolyGT3(void* s, DIVPOLYGON3* divp) {
    return div3(s, divp, DIV_TEXTURED | DIV_GOURAUD);
}
u_long* RCpolyF4(void* s, DIVPOLYGON4* divp) { return div4(s, divp, 0); }
u_long* RCpolyFT4(void* s, DIVPOLYGON4* divp) {
    return div4(s, divp, DIV_TEXTURED);
}
u_long* RCpolyG4(void* s, DIVPOLYGON4* divp) {
    return div4(s, divp, DIV_GOURAUD);
}
u_long* RCpolyGT4(void* s, DIVPOLYGON4* divp) {
    return div4(s, divp, DIV_TEXTURED | DIV_GOURAUD);
}
