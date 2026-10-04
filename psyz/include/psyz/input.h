#ifndef PSYZ_INPUT_H
#define PSYZ_INPUT_H

/**
 * @file input.h
 * @brief Controller and gamepad input endpoints.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Controller identifiers to emulate multiple PS1 controller kinds
 *
 * https://problemkaputt.de/psxspx-controllers-communication-sequence.htm
 */
typedef enum {
    PSYZ_CTRL_ERROR = 0x00,         /**< Failed to set controller param */
    PSYZ_CTRL_QUERY_KIND = 0x00,    /**< Query current kind without changing it */
    PSYZ_CTRL_MOUSE = 0x12,         /**< TODO document buffer format */
    PSYZ_CTRL_DIGITAL_PAD = 0x41,   /**< TODO document buffer format */
    PSYZ_CTRL_ANALOG_STICK = 0x53,  /**< TODO document buffer format */
    PSYZ_CTRL_ANALOG_PAD = 0x73,    /**< TODO document buffer format */
    PSYZ_CTRL_KEYBOARD = 0x96,      /**< TODO document buffer format */
    PSYZ_CTRL_DISCONNECTED = 0xFF,  /**< TODO */
} PsyzControllerKind;

/**
 * @brief Select which controller kind a port emulates
 *
 * Sets the controller kind reported on @p port and @p channel.
 * Defaults to PSYZ_CTRL_DIGITAL_PAD for better compatibility with early games.
 * When the reserved PSYZ_CTRL_QUERY_KIND value is passed, fetch the current
 * controller kind for the specified @p port and @p channel as returned value.
 *
 * @param port Controller port (0 or 1)
 * @param channel Multitap channel (0-3). Currently reserved and unused.
 * @param kind Controller kind to emulate, or PSYZ_CTRL_QUERY_KIND
 * @return Previously selected kind, or PSYZ_CTRL_ERROR on invalid arguments
 */
PsyzControllerKind Psyz_PadsSetKind(
    int port, int channel, PsyzControllerKind kind);

/**
 * Size of the controller receive buffer the BIOS fills on real hardware:
 * byte 0 = status, byte 1 = controller kind, then the per-kind payload.
 * https://problemkaputt.de/psxspx-controllers-and-memory-cards.htm
 */
#define PSYZ_PAD_BUF_LEN 34

/**
 * @brief Retrieve the internal 34-byte controller frame for a port
 *
 * Copies the latest hardware-faithful SIO frame for @p port into @p dst.
 * Direct low-level entrypoint for libetc/libapi. Depending on the backend
 * implementation, triggers an OS poll of gamepad or mapped keyboard.
 *
 * @param port Controller port (0 or 1)
 * @param dst Buffer to write into
 * @param len Buffer length in bytes (at most PSYZ_PAD_BUF_LEN is copied)
 */
void Psyz_PadsGet(int port, char* dst, int len);

/**
 * @brief Set the internal 34-byte controller frame.
 *
 * Called by the platform input backend to store the same frame retrieved with
 * Psyz_PadsGet. It can be used to emulate input as part of automated tests.
 *
 * @param port Controller port (0 or 1)
 * @param src Buffer to write from
 * @param len Buffer length in bytes
 */
void Psyz_PadsSet(int port, const char* src, int len);

/**
 * @brief One key of pad 1's keyboard map
 *
 * While @p key is held, the pad reports @p buttons pressed. @p key is the
 * backend's key code: an SDL_Scancode on SDL3 (SDL_SCANCODE_W, ...), so the
 * binding follows the key's position, not the letter the layout prints on it.
 * @p buttons is a mask of libetc's PadRead bits (PADLup, PADRright, ...).
 * A key may press several buttons, and several keys the same button.
 */
typedef struct {
    int key;
    unsigned short buttons;
} PsyzKeyBinding;

/** The most bindings Psyz_PadsSetKeyboardMap takes. */
#define PSYZ_KEYBOARD_MAP_MAX 64

/**
 * @brief Replace the keyboard map of pad 1
 *
 * The keyboard always drives port 0; gamepads keep their own mapping. The
 * map is copied. A NULL @p map or a @p count of 0 restores the built-in map
 * (arrows d-pad, Enter START, Backspace SELECT, D circle, S triangle,
 * X cross, Z square, Q L1, W L2, E R2, R R1, 1/2 L3/R3).
 *
 * Escape quits the program while the map in use does not bind it; a game
 * that binds Escape (say, to START for a pause) quits through the window's
 * close button instead.
 *
 * @param map Bindings, at most PSYZ_KEYBOARD_MAP_MAX
 * @param count Number of bindings in @p map
 * @return @p count, 0 when the built-in map is restored, or -1 (map
 *         unchanged) when there are too many bindings or a key is invalid.
 *         Backends without a keyboard return -1.
 */
int Psyz_PadsSetKeyboardMap(const PsyzKeyBinding* map, int count);

#ifdef __cplusplus
}
#endif

#endif
