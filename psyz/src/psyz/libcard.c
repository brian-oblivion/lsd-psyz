#include <psyz.h>
#include <libapi.h>
#include <kernel.h>
#include <psyz/log.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

// The cards plugged in: one in each port (bu00, bu10), none on a multi tap.
// A card is "new" until something is written to it, as after power-on:
// _card_info answers EvSpNEW, and a game confirms the card with _card_clear.
// _bu_init leaves both known: on DuckStation, LSD: Dream Emulator's first
// _card_info after _bu_init answers a known card, not a new one.
static unsigned char card_confirmed[2];

static int card_present(long chan) { return (chan & 15) == 0; }

// 1:valid, 0:invalid
static inline int validate_chan(long chan) {
    unsigned int cardNo = (unsigned int)(chan & 15);
    unsigned int portNo = (unsigned int)(chan >> 4);
    if (cardNo >= 4) { // validate multi-tap card support
        return 0;
    }
    if (portNo >= 2) { // PS1 has only two memcard ports
        return 0;
    }
    return 1;
}

// Creates the directories that hold the two memory cards, where
// Psyz_AdjustPath maps "bu00:" and "bu10:" (so a game's Psyz_AdjustPathCB
// can put them anywhere). The parent directory must exist. Both cards are
// known afterwards (card_confirmed).
void _bu_init(void) {
    card_confirmed[0] = card_confirmed[1] = 1;
    static const char* const devices[] = {"bu00:", "bu10:"};
    for (size_t i = 0; i < sizeof(devices) / sizeof(*devices); i++) {
        char dir[0x100];
        Psyz_AdjustPath(dir, devices[i], sizeof(dir));
        size_t len = strlen(dir);
        while (len > 1 && (dir[len - 1] == '/' || dir[len - 1] == '\\')) {
            dir[--len] = '\0';
        }
#ifdef _WIN32
        _mkdir(dir);
#else
        mkdir(dir, 0755);
#endif
    }
}

long _card_auto(long val) {
    NOT_IMPLEMENTED;
    return val;
}

long _card_info(long chan) {
    if (!validate_chan(chan)) {
        return 0;
    }
    if (!card_present(chan)) {
        DeliverEvent(SwCARD, EvSpTIMOUT);
    } else if (!card_confirmed[chan >> 4]) {
        DeliverEvent(SwCARD, EvSpNEW);
    } else {
        DeliverEvent(SwCARD, EvSpIOE);
    }
    return 1;
}

// Every card present is formatted: its directory exists (_bu_init).
long _card_load(long chan) {
    if (!validate_chan(chan)) {
        return 0;
    }
    DeliverEvent(SwCARD, card_present(chan) ? EvSpIOE : EvSpTIMOUT);
    return 1;
}

// Keeps the next _card_read or _card_write from answering EvSpNEW, which on
// the host they never do.
void _new_card(void) {}

long _card_status(long drv) {
    NOT_IMPLEMENTED;
    return 0;
}

void InitCARD2(long val) { NOT_IMPLEMENTED; }

long StartCARD2(void) {
    NOT_IMPLEMENTED;
    return 0;
}

long StopCARD2(void) {
    NOT_IMPLEMENTED;
    return 0;
}

void _ExitCard(void) { NOT_IMPLEMENTED; }

static void _bzero(unsigned char* p, int n) { memset(p, 0, n); }

long _card_sector_write(long chan, long block, unsigned char* buf) {
    unsigned char sp10[0x80];
    u8 checksum;
    s32 retries;
    s32 i;
    u8* var_v1;
    u8* var_v1_2;

    retries = 0;
    var_v1 = buf;
    checksum = 0;
    for (i = 0; i < 0x7F; i++) {
        checksum ^= *var_v1++;
    }
    *var_v1 = checksum;
    while (1) {
        if (retries < 8) {
            _new_card();
            if (_card_write(chan, block, buf) == 1) {
                do {
                } while (!(_card_status(chan >> 4) & 1));
                var_v1_2 = sp10;
                _bzero(var_v1_2, sizeof(sp10));
                _new_card();
                if (_card_read(chan, block, var_v1_2) != 1) {
                    ERRORF("card read error\n");
                } else {
                    do {
                    } while (!(_card_status(chan >> 4) & 1));
                }
                var_v1 = sp10;
                checksum = 0;
                for (i = 0; i < 0x7F; i++) {
                    checksum ^= *var_v1++;
                }
                if (buf[0x7F] == checksum) {
                    return 1;
                }
                retries++;
            } else {
                return 0;
            }
        } else {
            break;
        }
    }
    return 0;
}

long _card_write(long chan, long block, unsigned char* buf) {
    // no sectors to write on the host, but the write confirms the card
    if (validate_chan(chan) && card_present(chan)) {
        card_confirmed[chan >> 4] = 1;
    }
    NOT_IMPLEMENTED;
    return 0;
}

long _card_read(long chan, long block, unsigned char* buf) {
    NOT_IMPLEMENTED;
    return 0;
}
