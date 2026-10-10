#ifndef PSYZ_GTE_H
#define PSYZ_GTE_H
#include <libgte.h>

/**
 * @file gte.h
 * @brief Geometry Transformation Engine (COP2) endpoints.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Read a GTE data register (COP2 data)
 *
 * The PS1 GTE (COP2) exposes 32 data registers holding input vectors,
 * accumulators, screen XY/Z FIFOs, and MAC values. Register indices follow
 * https://psx-spx.consoledev.net/geometrytransformationenginegte/
 *
 * @param reg Register index (0-31)
 * @return Register value packed as a 32-bit word
 */
unsigned int Psyz_GteDataRead(unsigned reg);

/**
 * @brief Write a GTE data register (COP2 data)
 *
 * Writes a 32-bit word into the GTE data register file. Used by an external
 * emulator to forward MTC2/LWC2 instructions into PsyZ's GTE state.
 * Register indices follow
 * https://psx-spx.consoledev.net/geometrytransformationenginegte/
 *
 * @param reg Register index (0-31)
 * @param value Value to write
 */
void Psyz_GteDataWrite(unsigned reg, unsigned int value);

/**
 * @brief Read a GTE control register (COP2 control)
 *
 * The GTE control registers hold transformation matrices, translation vectors,
 * projection parameters, and the FLAG register. Register indices follow
 * https://psx-spx.consoledev.net/geometrytransformationenginegte/
 *
 * @param reg Register index (0-31)
 * @return Register value packed as a 32-bit word
 */
unsigned int Psyz_GteCtrlRead(unsigned reg);

/**
 * @brief Write a GTE control register (COP2 control)
 *
 * Writes a 32-bit word into the GTE control register file. Used by an external
 * emulator to forward CTC2 instructions into PsyZ's GTE state.
 * Register indices follow
 * https://psx-spx.consoledev.net/geometrytransformationenginegte/
 *
 * @param reg Register index (0-31)
 * @param value Value to write
 */
void Psyz_GteCtrlWrite(unsigned reg, unsigned int value);

/**
 * @brief Execute a GTE command (COP2 instruction)
 *
 * Dispatches a 25-bit GTE command to the corresponding operation. Bits 0-5
 * select the operation (e.g. RTPS, RTPT, NCLIP, AVSZ3, AVSZ4). Used by an
 * external emulator to forward COP2 instructions into PsyZ's GTE state.
 * https://psx-spx.consoledev.net/geometrytransformationenginegte/
 *
 * On a real PlayStation it emits the COP2 instruction instead, so `cmd` must
 * be a compile-time constant there.
 *
 * @param cmd 25-bit GTE command word
 */
#ifdef __psyz
void Psyz_GteCommand(unsigned int cmd);
#else
#define Psyz_GteCommand(cmd)                                                   \
    __asm__ volatile("nop;"                                                    \
                     "nop;"                                                    \
                     ".word %0" ::"i"(0x4A000000 | (cmd)))
#endif

void Psyz_GteLdRgb(CVECTOR* v);
void Psyz_GteStRgb(CVECTOR* v);
void Psyz_GteLdRgb3(CVECTOR* v0, CVECTOR* v1, CVECTOR* v2);
void Psyz_GteLdRgb3c(CVECTOR* v);
void Psyz_GteStRgb3(CVECTOR* v0, CVECTOR* v1, CVECTOR* v2);
void Psyz_GteStRgb3G3(void* polyG3);
void Psyz_GteStRgb3Gt3(void* polyGt3);
void Psyz_GteStRgb3G4(void* polyG4);
void Psyz_GteStRgb3Gt4(void* polyGt4);
void Psyz_GteLdDp(long p);
void Psyz_GteLdSz3(long sz1, long sz2, long sz3);
void Psyz_GteLdSz4(long sz0, long sz1, long sz2, long sz3);
void Psyz_GteNccs(void);
void Psyz_GteLdClmv(void* p);
void Psyz_GteStClmv(void* p);
void Psyz_GteLdTr(long tx, long ty, long tz);
void Psyz_GteLdTx(long v);
void Psyz_GteLdTy(long v);
void Psyz_GteLdTz(long v);
void Psyz_GteAvsz3(void);
void Psyz_GteAvsz4(void);
void Psyz_GteDpcs(void);
void Psyz_GteLcir(void);
/**
 * @brief Scale the projected screen X around the screen offset (OFX)
 *
 * Not part of the PS1's GTE: an enhancement for displays wider than 4:3.
 * RTPS and RTPT (and the libgte calls built on them) multiply the projected
 * X by @p scale, 16.16 fixed point, before adding OFX. 0x10000 (the default)
 * is the console's projection; 0xC000 (3/4) squeezes a 16:9 field of view
 * into a 4:3 framebuffer, to be shown stretched with
 * Psyz_VideoSetDisplayStretch.
 *
 * @param scale 16.16 factor; 0 or less restores 0x10000
 */
void Psyz_GteSetScreenXScale(int scale);
int Psyz_GteGetScreenXScale(void);
/**
 * @brief The depth cue RTPS and RTPT give, from the host
 *
 * Not part of the PS1's GTE: an enhancement, for a fog of the host's own.
 * Called by RTPS and RTPT (and the libgte calls built on them) for the
 * vertex whose depth cue they leave in IR0 (RTPT's last), after the GTE's
 * own from DQA and DQB.
 *
 * @param x, y, z the vertex in view space (MAC1..MAC3 as sf 1 leaves them)
 * @param dp the GTE's depth cue, 0..0x1000 (IR0)
 * @param fade 0 on entry; set it to draw the vertices the command projected
 *        (RTPT's three, RTPS's one) that much transparent, up to 0x1000 (not
 *        at all). The GPU renderers draw a polygon made from them blended
 *        over what is behind it; the game never sees it.
 * @return the depth cue to leave in IR0; outside 0..0x1000 it is clamped
 *         and flagged as the GTE flags its own
 */
typedef int (*PsyzGteDepthCueHook)(int x, int y, int z, int dp, int* fade);
/**
 * @brief Set or clear (NULL, the default) the host's depth cue
 *
 * @return 1 when the hook's fade is drawn (a build with
 *         PSYZ_PRECISE_GEOMETRY), else 0
 */
int Psyz_GteSetDepthCueHook(PsyzGteDepthCueHook hook);
void Psyz_GteRtps(void);
void Psyz_GteRtpt(void);
void Psyz_GteNclip(void);
void Psyz_GteRt(void);
void Psyz_GteStlvnl(VECTOR* out);
void Psyz_GteRtir(void);
void Psyz_GteLdlv0(VECTOR* v);
void Psyz_GteStlvl(VECTOR* out);
void Psyz_GteLdopv1(VECTOR* v);
void Psyz_GteLdopv2(VECTOR* v);
void Psyz_GteOp12(void);
int Psyz_GteReadflg(void);
void Psyz_GteLdv0(SVECTOR* v);
void Psyz_GteLdv3(SVECTOR* v0, SVECTOR* v1, SVECTOR* v2);
void Psyz_GteLdv01c(SVECTOR* v);
void Psyz_GteLdv3c(SVECTOR* v);
void Psyz_GteStsxy(unsigned int* out);
void Psyz_GteStsxy3(unsigned int* out0, unsigned int* out1, unsigned int* out2);
void Psyz_GteStsxy01c(unsigned int* out);
void Psyz_GteStsxy3Gt3(void* polyGt3);
void Psyz_GteStsxy3G3(void* polyG3);
void Psyz_GteStsxy2(unsigned int* out);
void Psyz_GteStsxy3F3(void* polyF3);
void Psyz_GteStsxy3F4(void* polyF4);
void Psyz_GteStsxy3Ft3(void* polyFt3);
void Psyz_GteStsxy3Ft4(void* polyFt4);
void Psyz_GteStsxy3G4(void* polyG4);
void Psyz_GteStsxy3Gt4(void* polyGt4);
void Psyz_GteDpct(void);
void Psyz_GteNcds(void);
void Psyz_GteLlir(void);
void Psyz_GteStdp(unsigned int* out);
void Psyz_GteStszotz(unsigned int* out);
void Psyz_GteStotz(unsigned int* out);
void Psyz_GteStopz(int* out);
void Psyz_GteRtv0(void);
void Psyz_GteRtv1(void);
void Psyz_GteRtv2(void);
void Psyz_GteStsz(unsigned int* out);
void Psyz_GteStsz3(unsigned int* out0, unsigned int* out1, unsigned int* out2);
void Psyz_GteStsz4(unsigned int* out0, unsigned int* out1, unsigned int* out2,
                   unsigned int* out3);
void Psyz_GteStsz3c(unsigned int* out);
void Psyz_GteStsz4c(unsigned int* out);

#ifdef __cplusplus
}
#endif

#endif
