#ifndef PSYZ_SPU_H
#define PSYZ_SPU_H

/**
 * @file spu.h
 * @brief Platform-agnostic PS1 SPU emulation endpoints.
 */

#include <psyz/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * PS1 SPU constants (the SPU emulator is platform-agnostic; SDL or any other
 * audio backend pulls 44100 Hz stereo short frames from Psyz_SpuPullSamples).
 */
#define PSYZ_SPU_RAM_SIZE (512 * 1024) /**< must be a power of two */
#define PSYZ_SPU_NUM_VOICES 24         /**< match PS1 voice count */
#define PSYZ_SPU_SAMPLE_RATE 44100     /**< match fixed PS1 sample rate */

/**
 * @brief Initialize SPU emulation state
 *
 * Idempotent; safe to call multiple times.
 */
void Psyz_SpuInit(void);

/**
 * @brief Reset SPU state
 *
 * @param hot When non-zero, RAM contents are preserved across the reset
 *            (mirrors the PSX-Q "hot init" semantics for libspu).
 */
void Psyz_SpuReset(int hot);

/**
 * @brief Write one 16-bit value into the SPU register file
 *
 * Certain registers can trigger a side-effect. Please refer to psxspx docs for
 * SPU reference.
 *
 * @param reg_offset Offset relative to 0x1F801C00; valid range 0x000-0x1FF
 * @param value 16-bit value to write
 */
void Psyz_SpuWrite(unsigned int reg_offset, unsigned short value);

/**
 * @brief Read back one 16-bit value from the SPU register file
 *
 * Internally maps to SPU_RXX.
 *
 * @param reg_offset Offset relative to 0x1F801C00; valid range 0x000-0x1FF
 * @return 16-bit register value
 */
unsigned short Psyz_SpuRead(unsigned int reg_offset);

/**
 * @brief Read back the current SPU transfer address
 *
 * Maps to the xfer_addr register.
 *
 * @return Current transfer address
 */
unsigned int Psyz_SpuGetTransferAddr(void);

/**
 * @brief Set the SPU RAM transfer address
 *
 * The destination offset into the 512KB RAM. Maps to the PSX SPU register
 * 0x1F801DA6 (xfer_addr) divided by 8.
 *
 * @param addr Transfer address
 */
void Psyz_SpuSetTransferAddr(unsigned int addr);

/**
 * @brief Push one 16-bit word into the SPU transfer FIFO
 *
 * Maps to a write of SPU register 0x1F801DA8 (xfer_fifo). Each call deposits
 * the word at the current transfer address in SPU RAM and bumps the transfer
 * address by 2.
 *
 * @param word 16-bit word to push
 */
void Psyz_SpuFifoWrite(unsigned short word);

/**
 * @brief Faster bulk version of Psyz_SpuFifoWrite
 *
 * Bypasses individual writes. Uses xfer_addr as the destination address and
 * updates it at the end of the call.
 *
 * @param src Source buffer
 * @param size Number of bytes to transfer
 */
void Psyz_SpuFifoWriteBulk(const unsigned char* src, unsigned int size);

/**
 * @brief Read bytes from SPU RAM
 *
 * Wraps at 512 KB. Does not affect xfer_addr. Useful for debugging.
 *
 * @param offset Byte offset into SPU RAM
 * @param dst Destination buffer
 * @param size Number of bytes to read
 */
void Psyz_SpuMemRead(unsigned int offset, void* dst, unsigned int size);

/**
 * @brief Write bytes into SPU RAM
 *
 * Wraps at 512 KB. Does not affect xfer_addr. Useful for debugging.
 *
 * @param offset Byte offset into SPU RAM
 * @param src Source buffer
 * @param size Number of bytes to write
 */
void Psyz_SpuMemWrite(unsigned int offset, const void* src, unsigned int size);

/**
 * @brief Direct pointer to the 512 KB SPU RAM
 *
 * For tests and offline rendering.
 *
 * @return Pointer to SPU RAM
 */
unsigned char* Psyz_SpuGetRam(void);

/**
 * @brief Generate stereo 16-bit LE PCM frames into out (interleaved L, R)
 *
 * Internally used by the PsyZ Audio subsystem, it can also be used for offline
 * rendering and unit tests.
 *
 * @param out Output buffer
 * @param num_frames Number of stereo frames to generate
 */
void Psyz_SpuPullSamples(short* out, int num_frames);

/**
 * Voice groups, for the host's volume controls. libsnd puts a voice keyed by
 * a sequence in PSYZ_SPU_GROUP_SEQ and one keyed as a sound effect (SsUtKeyOn
 * and the like) in PSYZ_SPU_GROUP_SE; a voice nothing has put in a group is
 * in PSYZ_SPU_GROUP_SEQ.
 */
#define PSYZ_SPU_GROUP_SEQ 0
#define PSYZ_SPU_GROUP_SE 1
#define PSYZ_SPU_GROUPS 2

/**
 * @brief Put a voice in a group from its next key-on
 *
 * @param voice Voice, 0 to PSYZ_SPU_NUM_VOICES - 1
 * @param group PSYZ_SPU_GROUP_*
 */
void Psyz_SpuSetVoiceGroup(int voice, int group);

/**
 * @brief Scale what a group's voices are heard at, reverb included
 *
 * The gains apply to what is heard only: the voices' registers, envelopes
 * (ENVX) and the capture buffers stay as the console has them. A change
 * ramps over about 12 ms. 1.0, the default, leaves the mix as it is.
 *
 * @param group PSYZ_SPU_GROUP_*
 * @param gain 0.0 (silent) to 1.0
 */
void Psyz_SpuSetGroupGain(int group, float gain);

/**
 * @brief Scale CD audio and XA, as Psyz_SpuSetGroupGain does voices
 */
void Psyz_SpuSetCdGain(float gain);

/**
 * @brief Scale the whole mix, after the main volume and its clipping
 */
void Psyz_SpuSetMasterGain(float gain);

/**
 * How a voice is resampled to its pitch.
 */
typedef enum {
    PSYZ_SPU_INTERP_GAUSS, /**< the console's 4-tap gaussian (the default) */
    PSYZ_SPU_INTERP_CUBIC, /**< 4-point cubic (Catmull-Rom): brighter */
    PSYZ_SPU_INTERP_SINC,  /**< 8-tap windowed sinc: flattest */
} PsyzSpuInterp;

/**
 * @brief Choose how voices are resampled
 *
 * Changes what is heard only: the capture buffers of voices 1 and 3 keep the
 * console's gaussian.
 */
void Psyz_SpuSetInterpolation(PsyzSpuInterp interp);

#ifdef __cplusplus
}
#endif

#endif
