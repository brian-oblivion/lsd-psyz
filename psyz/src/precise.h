#ifndef PRECISE_H
#define PRECISE_H
#include <psyz/video.h>

// Precise geometry (Psyz_VideoSetGeometry): the GTE keeps the unrounded
// screen position and depth of every vertex it projects beside the 16-bit
// SXY the game sees, and each SXY store into memory leaves that precise
// vertex in a table keyed by the address it went to. When a polygon is
// queued for drawing, its XY words are looked up by address; a hit counts
// only when the word still holds the value that was stored with it, and was
// stored this frame or the last. Anything else (a value the game made up or
// changed, a copy through plain C, an old frame's leftover) is drawn from its
// 16-bit value, as the console draws it.
//
// The game never sees any of it: the GTE registers, the FLAG and every word
// it stores keep their console values.

typedef struct {
    float x, y; // screen position, in the 16-bit SXY's units
    float w;    // view-space depth (SZ's units), 0 when there is no precise
                // vertex
    float fade; // how transparent the host's depth cue wants the vertex
                // drawn, 0 (as the console) to 1 (Psyz_GteSetDepthCueHook)
} PreciseVertex;

// Psyz_VideoSetGeometry's mode; always the console's in a build without it.
extern PsyzGeometry precise_mode;
// A depth-cue hook is set: the vertices are carried for their fade, whatever
// the geometry.
extern int precise_fade;

#ifdef PSYZ_PRECISE_GEOMETRY
#define PRECISE_ON (precise_mode != PSYZ_GEOMETRY_CONSOLE)
// The vertices are carried to the GPU: for their positions, or their fade.
#define PRECISE_TRACK (PRECISE_ON || precise_fade)
#else
#define PRECISE_ON 0
#define PRECISE_TRACK 0
#endif

// Carries the vertices for their fade (on) or not; -1 when it can't (a
// build without PSYZ_PRECISE_GEOMETRY, or no memory for the table).
int Precise_SetFade(int on);

// The word at addr has just been stored from v (NULL: from no precise
// vertex, which forgets what an earlier store left there).
void Precise_Put(const void* addr, const PreciseVertex* v);

// The precise vertex stored at addr, if the word there still holds what was
// stored and it has a position or a fade; else out->w = out->fade = 0 and 0
// is returned.
int Precise_Get(const void* addr, PreciseVertex* out);

// The word at src was copied to dst: carry its precise vertex along.
void Precise_Copy(const void* dst, const void* src);

// Called once per presented frame, to age the table.
void Precise_NextFrame(void);

// The precise vertices of the words passed to Draw_PushPrim, one per word in
// the same order (NULL when the geometry is the console's). Backends that
// draw only integer positions ignore it.
extern const PreciseVertex* precise_draw_words;

#endif
