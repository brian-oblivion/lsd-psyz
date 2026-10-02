#ifndef LIBGS_H
#define LIBGS_H
#include <libgpu.h>
#include <libgte.h>

/**
 * @file libgs.h
 * @brief Graphics System Library
 *
 * This library provides high-level 3D graphics support, built on top of libgpu
 * and libgte. It offers automatic coordinate transformation, lighting
 * calculations, and primitive sorting.
 *
 * Key features:
 * - Automatic GTE (Geometry Transform Engine) coordinate calculations
 * - Light source management (ambient, parallel, point light)
 * - Fog effects and lighting modes
 * - Ordering table (OT) management
 * - Screen coordinate transformation
 * - Packet area management for primitives
 * - World/Screen/Light matrix handling
 */

/* Constants */
#define GsOFSGTE 0
#define GsOFSGPU 4
#define GsINTER 1
#define GsNONINTER 0
#define GsRESET0 0
#define GsRESET3 (3 << 4)

/* Lighting modes */
#define GsLMODE_NORMAL 0     /**< Normal mode */
#define GsLMODE_FOG 1        /**< Fog only mode */
#define GsLMODE_LOFF 2       /**< Light source calculation off */
#define GsLMODE_NORMAL_FOG 3 /**< Normal + fog */

/* Attribute bit masks (GsDOBJ2, GsSPRITE, GsBG, ... attribute) */
#define GsFOG (1 << 3)     /**< Fog on (3D objects) */
#define GsMATE (1 << 4)    /**< Material colour on (3D objects) */
#define GsLLMOD (1 << 5)   /**< Local lighting mode */
#define GsLOFF (1 << 6)    /**< Light off */
#define GsDIV1 (1 << 9)    /**< Subdivide polygons 2x2 */
#define GsDIV2 (2 << 9)    /**< Subdivide polygons 4x4 */
#define GsDIV3 (3 << 9)    /**< Subdivide polygons 8x8 */
#define GsDIV4 (4 << 9)    /**< Subdivide polygons 16x16 */
#define GsDIV5 (5 << 9)    /**< Subdivide polygons 32x32 */
#define GsROTOFF (1 << 27) /**< Sprite rotation and scaling off */
#define GsAZERO (0 << 28)  /**< Semi-transparency rate: 50% back + 50% front */
#define GsAONE (1 << 28)   /**< Semi-transparency rate: 100% back + 100% front */
#define GsATWO (2 << 28)   /**< Semi-transparency rate: 100% back - 100% front */
#define GsATHREE (3 << 28) /**< Semi-transparency rate: 100% back + 25% front */
#define GsALON (1 << 30)   /**< Semi-transparency on */
#define GsAON GsALON       /**< Semi-transparency on (older psyz name) */
#define GsDOFF (1 << 31)   /**< Display off */

/* GsTMDFlag bits: how TMD data was preprocessed */
#define GsTMDFlagGRD 0x04 /**< Gradation (per-vertex colour) polygons */

/* GPU command codes of the primitives libgs builds from TMD data */
#define GPU_COM_F3 0x20   /**< Flat triangle */
#define GPU_COM_TF3 0x24  /**< Flat textured triangle */
#define GPU_COM_G3 0x30   /**< Gouraud triangle */
#define GPU_COM_TG3 0x34  /**< Gouraud textured triangle */
#define GPU_COM_F4 0x28   /**< Flat quad */
#define GPU_COM_TF4 0x2c  /**< Flat textured quad */
#define GPU_COM_G4 0x38   /**< Gouraud quad */
#define GPU_COM_TG4 0x3c  /**< Gouraud textured quad */
#define GPU_COM_NF3 0x21  /**< Flat triangle, no lighting */
#define GPU_COM_NTF3 0x25 /**< Flat textured triangle, no lighting */
#define GPU_COM_NG3 0x31  /**< Gouraud triangle, no lighting */
#define GPU_COM_NTG3 0x35 /**< Gouraud textured triangle, no lighting */
#define GPU_COM_NF4 0x29  /**< Flat quad, no lighting */
#define GPU_COM_NTF4 0x2d /**< Flat textured quad, no lighting */
#define GPU_COM_NG4 0x39  /**< Gouraud quad, no lighting */
#define GPU_COM_NTG4 0x3d /**< Gouraud textured quad, no lighting */

typedef unsigned char PACKET;

/**
 * @brief Ordering table tag
 */
typedef struct {
#ifndef __psyz
    unsigned p : 24;
    unsigned char num : 8;
#else
    O_TAG;
#endif
} GsOT_TAG;

/**
 * @brief Ordering table structure
 *
 * Manages the ordering table used for depth sorting of graphics primitives.
 */
typedef struct {
    unsigned int length; /**< Number of OT entries */
    GsOT_TAG* org;       /**< Pointer to OT buffer */
    unsigned int offset; /**< Z offset value */
    unsigned int point;  /**< Current registration position */
    GsOT_TAG* tag;       /**< Work pointer */
} GsOT;

/**
 * @brief Coordinate parameter format
 *
 * Used to retain information for GsCOORDINATE2 when TOD animation is used.
 */
typedef struct {
    VECTOR scale;   /**< Coordinate scaling information */
    SVECTOR rotate; /**< Coordinate rotation information */
    VECTOR trans;   /**< Coordinate parallel shift information */
} GsCOORD2PARAM;

/**
 * @brief Matrix type coordinate system
 *
 * Has superior coordinates and is defined by the matrix type coord. workm
 * retains the result of multiplication of matrices performed by GsGetLw() and
 * GsGetLs().
 */
typedef struct _GsCOORDINATE2 {
    u_long flg;           /**< Flag indicating whether coord was rewritten */
    MATRIX coord;         /**< Matrix */
    MATRIX workm;         /**< Result of multiplication to WORLD coordinates */
    GsCOORD2PARAM* param; /**< Pointer for scale, rotation, and transferq */
    struct _GsCOORDINATE2* super; /**< Pointer to superior coordinates */
    struct _GsCOORDINATE2* sub;   /**< Not in current use */
} GsCOORDINATE2;

/**
 * @brief Three-dimensional object handler
 *
 * Used to manipulate objects in a three-dimensional model. Use GsLinkObject4()
 * to link to TMD-format model data. Use GsSortObject4() to register in the
 * ordering table.
 */
typedef struct {
    u_long attribute;      /**< Object attribute (32-bit) */
    GsCOORDINATE2* coord2; /**< Pointer to local coordinate system */
    u_long* tmd;           /**< Pointer to model data */
    u_long id;             /**< Reserved by layout tool */
} GsDOBJ2;

/**
 * @brief Three-dimensional object handler for PMD format
 *
 * Used with PMD format model data. Use GsLinkObject3() to link to PMD file
 * model data. Use GsSortObject3() to register in the ordering table.
 */
typedef struct {
    u_long attribute;      /**< Object attribute (32-bit) */
    GsCOORDINATE2* coord2; /**< Pointer to local coordinate system */
    u_long* pmd;           /**< Pointer to model data (PMD format) */
    u_long* base;          /**< Pointer to object base address */
    u_long* sv;            /**< Pointer to shared vertex base address */
    u_long id;             /**< Reserved by layout tool */
} GsDOBJ3;

/**
 * @brief Three-dimensional object handler for use with GsSortObject5()
 *
 * Use GsLinkObject5() to link to TMD file model data. Use GsSortObject5() to
 * register in the ordering table. Supports preset packets.
 */
typedef struct {
    u_long attribute;      /**< Object attribute (32-bit) */
    GsCOORDINATE2* coord2; /**< Pointer to local coordinate system */
    u_long* tmd;           /**< Pointer to model data */
    u_long* packet;        /**< Pointer to preset packet area */
    u_long id;             /**< Reserved by layout tool */
} GsDOBJ5;

/**
 * @brief Cells constituting BG
 *
 * A rectangular array of GsCELL structures describes individual cells that fit
 * together to create a BG.
 */
typedef struct {
    u_char u;      /**< Offset (X-direction) within the page */
    u_char v;      /**< Offset (Y-direction) within the page */
    u_short cba;   /**< CLUT ID */
    u_short flag;  /**< Drawing options (flip flags) */
    u_short tpage; /**< Texture page number */
} GsCELL;

/**
 * @brief Map structure for BG
 *
 * Describes the mapping of cells for background surfaces.
 */
typedef struct {
    u_char cellw;   /**< Cell width */
    u_char cellh;   /**< Cell height */
    u_short ncellw; /**< Number of cells in width */
    u_short ncellh; /**< Number of cells in height */
    GsCELL* base;   /**< Pointer to cell array */
    u_short* index; /**< Pointer to map index */
} GsMAP;

/**
 * @brief BG (background surface) handler
 *
 * A BG is drawn as a large rectangle based on GsMAP data on a combination of
 * small rectangles defined by GsCELL data. Use GsSortBg() to register in the
 * ordering table.
 */
typedef struct {
    u_long attribute;       /**< Attribute */
    short x, y;             /**< Top left point display position */
    short w, h;             /**< BG display size */
    short scrollx, scrolly; /**< X and Y scroll values */
    u_char r, g, b;         /**< Display brightness (128 = normal) */
    GsMAP* map;             /**< Pointer to map data */
    short mx, my; /**< Rotation and enlargement central point coordinates */
    short scalex, scaley; /**< Scale values in X and Y directions */
    int rotate;           /**< Rotation angle (4096 = 1 degree) */
} GsBG;

/**
 * @brief Rectangle handler
 *
 * Used to draw a rectangle in a single color. Use GsSortBoxFill() to register
 * in the ordering table.
 */
typedef struct {
    u_long attribute; /**< Attribute */
    short x, y;       /**< Display position (top left point) */
    u_short w, h;     /**< Size of rectangle (width, height) */
    u_char r, g, b;   /**< Drawing color */
} GsBOXF;

/**
 * @brief Line handler
 *
 * Used to draw lines. Use GsSortLine() to register in the ordering table.
 */
typedef struct {
    u_long attribute; /**< Attribute */
    short x0, y0;     /**< Start point */
    short x1, y1;     /**< End point */
    u_char r, g, b;   /**< Drawing color */
} GsLINE;

/**
 * @brief Gouraud-shaded line handler
 *
 * Used to draw Gouraud-shaded lines. Use GsSortGLine() to register in the
 * ordering table.
 */
typedef struct {
    u_long attribute;  /**< Attribute */
    short x0, y0;      /**< Start point */
    short x1, y1;      /**< End point */
    u_char r0, g0, b0; /**< Start point color */
    u_char r1, g1, b1; /**< End point color */
} GsGLINE;

/**
 * @brief Sprite handler
 *
 * Used to draw sprites. Use GsSortSprite() to register in the ordering table.
 */
typedef struct {
    u_long attribute;     /**< Attribute */
    short x, y;           /**< Display position */
    short w, h;           /**< Sprite size */
    u_short tpage;        /**< Texture page ID */
    u_char u, v;          /**< Texture coordinates */
    short cx, cy;         /**< Rotation center */
    u_char r, g, b;       /**< Color */
    short mx, my;         /**< Enlargement center */
    short scalex, scaley; /**< Scale values */
    int rotate;           /**< Rotation angle */
} GsSPRITE;

/**
 * @brief TIM image information
 *
 * Filled by GsGetTimInfo() from TIM data: where the pixel data and the CLUT
 * go in VRAM, their sizes, and where they are in the TIM.
 */
typedef struct {
    u_long pmode;   /**< Pixel mode (bits 0-2) and CLUT flag (bit 3) */
    short px, py;   /**< Pixel data VRAM position */
    u_short pw, ph; /**< Pixel data size (in 16-bit units) */
    u_long* pixel;  /**< Pointer to pixel data */
    short cx, cy;   /**< CLUT VRAM position */
    u_short cw, ch; /**< CLUT size */
    u_long* clut;   /**< Pointer to CLUT data */
} GsIMAGE;

/**
 * @brief Fog parameter
 *
 * Used to set fog parameters with GsSetFogParam().
 */
typedef struct {
    short dqa;            /**< Fog coefficient A */
    int dqb;              /**< Fog coefficient B */
    u_char rfc, gfc, bfc; /**< Fog color (R, G, B) */
} GsFOGPARAM;

/**
 * @brief Flat light source
 *
 * Used to set flat light source with GsSetFlatLight().
 */
typedef struct {
    int vx, vy, vz; /**< Light direction vector */
    u_char r, g, b; /**< Light color */
} GsF_LIGHT;

/**
 * @brief Viewpoint position (reference type)
 *
 * Contains viewpoint information. Set with GsSetRefView2(). The viewpoint
 * coordinates in the coordinate system displayed by super are set in vpx, vpy,
 * vpz. The reference point coordinates are set in vrx, vry, vrz.
 */
typedef struct {
    int vpx, vpy, vpz; /**< Viewpoint coordinates */
    int vrx, vry, vrz; /**< Reference point coordinates */
    int rz;            /**< Viewpoint twist */
    GsCOORDINATE2*
        super; /**< Pointer to coordinate system which sets viewpoint */
} GsRVIEW2;

/**
 * @brief Viewpoint position (matrix type)
 *
 * Sets the viewpoint coordinates used by libgs. Directly specifies the matrix
 * used to change from parent coordinates to viewpoint coordinates. Set with
 * GsSetView2().
 */
typedef struct {
    MATRIX view; /**< Matrix from parent coordinates to viewpoint coordinates */
    GsCOORDINATE2*
        super; /**< Pointer to coordinate system which sets viewpoint */
} GsVIEW2;

/**
 * @brief Object table for TOD
 *
 * Used to manage multiple objects for TOD animation.
 */
typedef struct {
    GsDOBJ2* top; /**< Pointer to object array */
    int nobj;     /**< Number of objects in use */
    int maxobj;   /**< Number of objects in the array */
} GsOBJTABLE2;

/**
 * @brief Z clipping range
 */
typedef struct {
    u_long farz;  /**< Far clip distance */
    u_long nearz; /**< Near clip distance */
} GsZCLIP;

/*
 * TMD primitives: the packet formats of a TMD file's primitive section, as
 * the file stores them (see the TMD format's documentation). A primitive is
 * a 4-byte header (out, in, dummy/ilen, cd/mode) followed by its fields;
 * the name says which: F flat, G gouraud, T textured, N no normals (no
 * lighting), 3/4 the vertex count, a trailing G per-vertex colours. Vertex
 * and normal fields are indices into the object's tables, and fields named
 * p, dummy or pN are padding.
 */

typedef struct {
    u_char out, in, dummy, cd;
    u_char r0, g0, b0, code;
    u_short n0, v0;
    u_short v1, v2;
} TMD_P_F3;

typedef struct {
    u_char out, in, dummy, cd;
    u_char r0, g0, b0, code;
    u_short n0, v0;
    u_short n1, v1;
    u_short n2, v2;
} TMD_P_G3;

typedef struct {
    u_char out, in, dummy, cd;
    u_char r0, g0, b0, code;
    u_char r1, g1, b1, dummy1;
    u_char r2, g2, b2, dummy2;
    u_short n0, v0;
    u_short v1, v2;
} TMD_P_F3G;

typedef struct {
    u_char out, in, dummy, cd;
    u_char r0, g0, b0, code;
    u_char r1, g1, b1, dummy1;
    u_char r2, g2, b2, dummy2;
    u_short n0, v0;
    u_short n1, v1;
    u_short n2, v2;
} TMD_P_G3G;

typedef struct {
    u_char out, in, dummy, cd;
    u_char r0, g0, b0, code;
    u_short v0, v1;
    u_short v2, p;
} TMD_P_NF3;

typedef struct {
    u_char out, in, dummy, cd;
    u_char r0, g0, b0, code;
    u_char r1, g1, b1, p1;
    u_char r2, g2, b2, p2;
    u_short v0, v1;
    u_short v2, p;
} TMD_P_NG3;

typedef struct {
    u_char out, in, dummy, cd;
    u_char r0, g0, b0, code;
    u_short n0, v0;
    u_short v1, v2;
    u_short v3, p;
} TMD_P_F4;

typedef struct {
    u_char out, in, dummy, cd;
    u_char r0, g0, b0, code;
    u_short n0, v0;
    u_short n1, v1;
    u_short n2, v2;
    u_short n3, v3;
} TMD_P_G4;

typedef struct {
    u_char out, in, dummy, cd;
    u_char r0, g0, b0, code;
    u_short v0, v1;
    u_short v2, v3;
} TMD_P_NF4;

typedef struct {
    u_char out, in, dummy, cd;
    u_char r0, g0, b0, code;
    u_char r1, g1, b1, p1;
    u_char r2, g2, b2, p2;
    u_char r3, g3, b3, p3;
    u_short v0, v1;
    u_short v2, v3;
} TMD_P_NG4;

typedef struct {
    u_char out, in, dummy, cd;
    u_char tu0, tv0;
    u_short clut;
    u_char tu1, tv1;
    u_short tpage;
    u_char tu2, tv2;
    u_short p;
    u_short n0, v0;
    u_short v1, v2;
} TMD_P_TF3;

typedef struct {
    u_char out, in, dummy, cd;
    u_char tu0, tv0;
    u_short clut;
    u_char tu1, tv1;
    u_short tpage;
    u_char tu2, tv2;
    u_short p;
    u_short n0, v0;
    u_short n1, v1;
    u_short n2, v2;
} TMD_P_TG3;

typedef struct {
    u_char out, in, dummy, cd;
    u_char tu0, tv0;
    u_short clut;
    u_char tu1, tv1;
    u_short tpage;
    u_char tu2, tv2;
    u_short p0;
    u_char r0, g0, b0, p1;
    u_short v0, v1;
    u_short v2, p2;
} TMD_P_TNF3;

typedef struct {
    u_char out, in, dummy, cd;
    u_char tu0, tv0;
    u_short clut;
    u_char tu1, tv1;
    u_short tpage;
    u_char tu2, tv2;
    u_short p0;
    u_char r0, g0, b0, p1;
    u_char r1, g1, b1, p2;
    u_char r2, g2, b2, p3;
    u_short v0, v1;
    u_short v2, p4;
} TMD_P_TNG3;

typedef struct {
    u_char out, in, dummy, cd;
    u_char tu0, tv0;
    u_short clut;
    u_char tu1, tv1;
    u_short tpage;
    u_char tu2, tv2;
    u_short p0;
    u_char tu3, tv3;
    u_short p1;
    u_short n0, v0;
    u_short v1, v2;
    u_short v3, p2;
} TMD_P_TF4;

typedef struct {
    u_char out, in, dummy, cd;
    u_char tu0, tv0;
    u_short clut;
    u_char tu1, tv1;
    u_short tpage;
    u_char tu2, tv2;
    u_short p0;
    u_char tu3, tv3;
    u_short p1;
    u_short n0, v0;
    u_short n1, v1;
    u_short n2, v2;
    u_short n3, v3;
} TMD_P_TG4;

typedef struct {
    u_char out, in, dummy, cd;
    u_char tu0, tv0;
    u_short clut;
    u_char tu1, tv1;
    u_short tpage;
    u_char tu2, tv2;
    u_short p0;
    u_char tu3, tv3;
    u_short p1;
    u_char r0, g0, b0, p2;
    u_short v0, v1;
    u_short v2, v3;
} TMD_P_TNF4;

typedef struct {
    u_char out, in, dummy, cd;
    u_char tu0, tv0;
    u_short clut;
    u_char tu1, tv1;
    u_short tpage;
    u_char tu2, tv2;
    u_short p0;
    u_char tu3, tv3;
    u_short p1;
    u_char r0, g0, b0, p2;
    u_char r1, g1, b1, p3;
    u_char r2, g2, b2, p4;
    u_char r3, g3, b3, p5;
    u_short v0, v1;
    u_short v2, v3;
} TMD_P_TNG4;

/**
 * @brief One TMD object's tables, as the GsTMDfast and GsTMDdiv drawing
 * functions take them
 */
struct TMD_STRUCT {
    u_long* vertop;  /**< Vertex table */
    u_long vern;     /**< Number of vertices */
    u_long* nortop;  /**< Normal table */
    u_long norn;     /**< Number of normals */
    u_long* primtop; /**< Primitive section */
    u_long primn;    /**< Number of primitives */
    u_long scale;    /**< Scale (unused by libgs) */
};

/* Globals */

extern MATRIX GsIDMATRIX;      /**< Identity matrix, translation zero */
extern PACKET* GsOUT_PACKET_P; /**< Packet work area (GsSetWorkBase) */
extern int GsLIGHT_MODE;       /**< Lighting mode (GsSetLightMode) */

/* Function declarations */

/**
 * @brief Clear V-BLANK count
 *
 * Clears the V-BLANK counter to zero.
 */
void GsClearVcount(void);

/**
 * @brief Get V-BLANK count
 *
 * Returns the number of V-BLANKs since the last call to GsClearVcount() or
 * GsInitVcount().
 *
 * @return V-BLANK count
 */
long GsGetVcount(void);

/**
 * @brief Initialize V-BLANK count
 *
 * Initializes the V-BLANK counter and starts counting.
 */
void GsInitVcount(void);

/**
 * @brief Clear ordering table
 *
 * Clears the ordering table and sets the offset and point values.
 *
 * @param offset Z offset value
 * @param point Current registration position
 * @param otp Pointer to ordering table
 */
void GsClearOt(unsigned short offset, unsigned short point, GsOT* otp);

/**
 * @brief Initialize graphics system
 *
 * Initializes the graphics system with the specified parameters.
 *
 * @param x Display area X position
 * @param y Display area Y position
 * @param intmode Interlace mode (GsINTER or GsNONINTER)
 * @param dith Dithering flag (0: off, 1: on)
 * @param varmmode Video mode (GsRESET0 or GsRESET3)
 */
void GsInitGraph(unsigned short x, unsigned short y, unsigned short intmode,
                 unsigned short dith, unsigned short varmmode);

/**
 * @brief Define double buffer
 *
 * Sets up double buffering with the specified coordinates for two buffers.
 *
 * @param x0 Buffer 0 X position
 * @param y0 Buffer 0 Y position
 * @param x1 Buffer 1 X position
 * @param y1 Buffer 1 Y position
 */
void GsDefDispBuff(
    unsigned short x0, unsigned short y0, unsigned short x1, unsigned short y1);

/**
 * @brief Get active buffer
 *
 * Returns which buffer is currently active for drawing.
 *
 * @return 0 or 1 indicating active buffer
 */
int GsGetActiveBuff(void);

/**
 * @brief Set work base address
 *
 * Sets the base address for the packet work area.
 *
 * @param outpacketp Pointer to packet buffer
 */
void GsSetWorkBase(PACKET* outpacketp);

/**
 * @brief Swap display buffer
 *
 * Swaps the display and drawing buffers for double buffering.
 */
void GsSwapDispBuff(void);

/**
 * @brief Sort clear primitive
 *
 * Registers a clear screen primitive to the ordering table.
 *
 * @param r Red component
 * @param g Green component
 * @param b Blue component
 * @param otp Pointer to ordering table
 */
void GsSortClear(unsigned char r, unsigned char g, unsigned char b, GsOT* otp);

/**
 * @brief Draw ordering table
 *
 * Draws all primitives in the ordering table.
 *
 * @param ot Pointer to ordering table
 */
void GsDrawOt(GsOT* ot);

/**
 * @brief Set drawing buffer clip
 *
 * Sets the drawing clip area to match the current drawing buffer.
 */
void GsSetDrawBuffClip(void);

/**
 * @brief Set drawing buffer offset
 *
 * Sets the drawing offset to match the current drawing buffer.
 */
void GsSetDrawBuffOffset(void);

/**
 * @brief Initialize 3D graphics
 *
 * Initializes the 3D graphics system.
 */
void GsInit3D(void);

/**
 * @brief Initialize coordinate system
 *
 * Initializes base to the identity, with super as its parent coordinate
 * system (WORLD, that is NULL, for the world).
 *
 * @param super Pointer to the parent coordinate system
 * @param base Pointer to coordinate system to initialize
 */
void GsInitCoordinate2(GsCOORDINATE2* super, GsCOORDINATE2* base);

/**
 * @brief Calculate local screen matrix
 *
 * Calculates the local screen perspective transformation matrix.
 *
 * @param coord Local coordinates
 * @param m Output matrix
 */
void GsGetLs(GsCOORDINATE2* coord, MATRIX* m);

/**
 * @brief Calculate local world matrix
 *
 * Calculates the local world matrix from the coordinate system.
 *
 * @param coord Local coordinates
 * @param m Output matrix
 */
void GsGetLw(GsCOORDINATE2* coord, MATRIX* m);

/**
 * @brief Calculate local world and screen matrices
 *
 * Calculates both local world and local screen matrices. Faster than calling
 * GsGetLw() and GsGetLs() separately.
 *
 * @param coord Pointer to local coordinates
 * @param lw Output local world matrix
 * @param ls Output local screen matrix
 */
void GsGetLws(GsCOORDINATE2* coord, MATRIX* lw, MATRIX* ls);

/**
 * @brief Set viewpoint (reference type)
 *
 * Calculates GsWSMATRIX from viewpoint information.
 *
 * @param pv Viewpoint position information
 * @return 0 on success
 */
int GsSetRefView2(GsRVIEW2* pv);

/**
 * @brief Set viewpoint (reference type, high precision)
 *
 * High precision version of GsSetRefView2().
 *
 * @param pv Viewpoint position information
 * @return 0 on success
 */
int GsSetRefView2L(GsRVIEW2* pv);

/**
 * @brief Set viewpoint (matrix type)
 *
 * Directly sets GsWSMATRIX from a matrix.
 *
 * @param pv Viewpoint position information
 * @return 0 on success
 */
int GsSetView2(GsVIEW2* pv);

/**
 * @brief Link object to TMD data (version 4)
 *
 * Links a GsDOBJ2 structure to object n of TMD-format model data that
 * GsMapModelingData() has mapped.
 *
 * @param tmd_base Address of the TMD data's first object
 * @param objp Pointer to object handler
 * @param n Object number in the TMD data
 */
void GsLinkObject4(u_long tmd_base, GsDOBJ2* objp, int n);

/**
 * @brief Link object to PMD data (version 3)
 *
 * Links a GsDOBJ3 structure to PMD-format model data.
 *
 * @param pmd_base Address of the PMD data's first object
 * @param objp Pointer to object handler
 * @return Address after the object's data
 */
u_long GsLinkObject3(u_long pmd_base, GsDOBJ3* objp);

/**
 * @brief Link object to TMD data (version 5)
 *
 * Links a GsDOBJ5 structure to object n of TMD-format model data, for use
 * with preset packets.
 *
 * @param tmd_base Address of the TMD data's first object
 * @param objp Pointer to object handler
 * @param n Object number in the TMD data
 */
void GsLinkObject5(u_long tmd_base, GsDOBJ5* objp, int n);

/**
 * @brief Sort 3D object to OT (version 4)
 *
 * Performs perspective transformation and light source calculation on a
 * GsDOBJ2 object and registers it to the ordering table.
 *
 * @param objp Pointer to object handler
 * @param otp Pointer to ordering table
 * @param shift Right shift from the object's Z to its OT position
 * @param scratch Work area, normally the scratchpad
 */
void GsSortObject4(GsDOBJ2* objp, GsOT* otp, int shift, u_long* scratch);

/**
 * @brief Sort 3D object to OT (version 3)
 *
 * Sorts a GsDOBJ3 object to the ordering table.
 *
 * @param objp Pointer to object handler
 * @param otp Pointer to ordering table
 * @param shift Right shift from the object's Z to its OT position
 */
void GsSortObject3(GsDOBJ3* objp, GsOT* otp, int shift);

/**
 * @brief Sort 3D object to OT (version 5)
 *
 * Sorts a GsDOBJ5 object (with preset packets) to the ordering table.
 *
 * @param objp Pointer to object handler
 * @param otp Pointer to ordering table
 * @param shift Right shift from the object's Z to its OT position
 * @param scratch Work area, normally the scratchpad
 */
void GsSortObject5(GsDOBJ5* objp, GsOT* otp, int shift, u_long* scratch);

/**
 * @brief Sort background to OT
 *
 * Registers a GsBG background to the ordering table.
 *
 * @param bg Pointer to background handler
 * @param otp Pointer to ordering table
 * @param pri Position in the ordering table (shifted by the OT's length)
 */
void GsSortBg(GsBG* bg, GsOT* otp, unsigned short pri);

/**
 * @brief Sort box fill to OT
 *
 * Registers a GsBOXF rectangle to the ordering table.
 *
 * @param boxf Pointer to rectangle handler
 * @param otp Pointer to ordering table
 * @param pri Position in the ordering table (shifted by the OT's length)
 */
void GsSortBoxFill(GsBOXF* boxf, GsOT* otp, unsigned short pri);

/**
 * @brief Sort line to OT
 *
 * Registers a GsLINE to the ordering table.
 *
 * @param line Pointer to line handler
 * @param otp Pointer to ordering table
 * @param pri Position in the ordering table (shifted by the OT's length)
 */
void GsSortLine(GsLINE* line, GsOT* otp, unsigned short pri);

/**
 * @brief Sort Gouraud line to OT
 *
 * Registers a GsGLINE to the ordering table.
 *
 * @param gline Pointer to Gouraud line handler
 * @param otp Pointer to ordering table
 * @param pri Position in the ordering table (shifted by the OT's length)
 */
void GsSortGLine(GsGLINE* gline, GsOT* otp, unsigned short pri);

/**
 * @brief Sort sprite to OT
 *
 * Registers a GsSPRITE to the ordering table.
 *
 * @param sprite Pointer to sprite handler
 * @param otp Pointer to ordering table
 * @param pri Position in the ordering table (shifted by the OT's length)
 */
void GsSortSprite(GsSPRITE* sprite, GsOT* otp, unsigned short pri);

/**
 * @brief Set light matrix
 *
 * Sets the light matrix for light source calculations.
 *
 * @param mp Pointer to light matrix
 */
void GsSetLightMatrix(MATRIX* mp);

/**
 * @brief Set ambient light
 *
 * Sets the ambient light color.
 *
 * @param r Red component
 * @param g Green component
 * @param b Blue component
 */
void GsSetAmbient(long r, long g, long b);

/**
 * @brief Set flat light
 *
 * Sets a flat (directional) light source.
 *
 * @param id Light source ID (0-2)
 * @param light Pointer to light source data
 * @return 0 on success
 */
int GsSetFlatLight(int id, GsF_LIGHT* light);

/**
 * @brief Set fog parameter
 *
 * Sets fog parameters for fog rendering.
 *
 * @param fogp Pointer to fog parameters
 */
void GsSetFogParam(GsFOGPARAM* fogp);

/**
 * @brief Set projection distance
 *
 * Sets the distance from the viewpoint to the projection plane.
 *
 * @param h Projection distance
 */
void GsSetProjection(long h);

/**
 * @brief Set screen offset
 *
 * Sets the screen offset for 2D drawing.
 *
 * @param x X offset
 * @param y Y offset
 */
void GsSetOffset(long x, long y);

/**
 * @brief Set screen origin
 *
 * Sets the screen origin for coordinate transformations.
 *
 * @param x X origin
 * @param y Y origin
 */
void GsSetOrign(long x, long y);

/**
 * @brief Set clip region
 *
 * Sets the 3D clipping region.
 *
 * @param clip Pointer to clip parameters
 */
void GsSetClip(RECT* clip);

/**
 * @brief Map modeling data
 *
 * Maps TMD modeling data offsets to actual memory addresses.
 *
 * @param base Pointer to TMD data
 */
void GsMapModelingData(u_long* base);

/**
 * @brief Get work base address
 *
 * Gets the current packet work area base address.
 *
 * @return Pointer to packet buffer
 */
PACKET* GsGetWorkBase(void);

/**
 * @brief Multiply coordinate matrices
 *
 * Multiplies two coordinate matrices, rotation and translation: m2 = m1 *
 * m2.
 *
 * @param m1 Pointer to first matrix
 * @param m2 Pointer to second matrix (input/output)
 */
void GsMulCoord2(MATRIX* m1, MATRIX* m2);

/**
 * @brief Set lighting mode
 *
 * Sets the default lighting mode for objects (GsLMODE_NORMAL, GsLMODE_FOG,
 * GsLMODE_LOFF), stored in GsLIGHT_MODE.
 *
 * @param mode Lighting mode
 */
void GsSetLightMode(int mode);

/**
 * @brief Set local screen matrix
 *
 * Sets the local screen matrix (from GsGetLs()) in the GTE as the rotation
 * and translation for the following perspective transforms.
 *
 * @param mp Pointer to local screen matrix
 */
void GsSetLsMatrix(MATRIX* mp);

/**
 * @brief Set near clip distance
 *
 * Polygons nearer than this are not drawn.
 *
 * @param clip_near Near clip distance
 */
void GsSetNearClip(long clip_near);

/**
 * @brief Set far clip distance
 *
 * Polygons farther than this are not drawn.
 *
 * @param clip_far Far clip distance
 */
void GsSetFarClip(long clip_far);

/**
 * @brief Get TIM image information
 *
 * Reads the header of TIM data and fills tim with where its pixels and CLUT
 * are, and where they go in VRAM.
 *
 * @param im Pointer to TIM data, after its ID word
 * @param tim Pointer to the image information to fill
 */
void GsGetTimInfo(u_long* im, GsIMAGE* tim);

#endif
