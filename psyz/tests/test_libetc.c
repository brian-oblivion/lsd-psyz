#include "ztest.h"
#include <psyz.h>
#include <kernel.h>
#include <libapi.h>
#include <libetc.h>
#include <psyz/timers.h>

static int interrupt_calls;
static void count_interrupt(void) { interrupt_calls++; }

ZTEST(interrupt_callback, sets_and_returns_the_vblank_handler) {
    zskip_targets("ps1");
    InterruptCallback(0, NULL);
    SetRCnt(RCntCNT3, 1, RCntMdINTR);
    zexpect_ptr_eq(NULL, InterruptCallback(0, count_interrupt));
    interrupt_calls = 0;
    Psyz_RcntAdd(44100);
    zexpect_s32_gt(0, interrupt_calls);
    zexpect_ptr_eq(count_interrupt, InterruptCallback(0, NULL));
}

ZTEST(interrupt_callback, null_removes_the_handler) {
    zskip_targets("ps1");
    SetRCnt(RCntCNT3, 1, RCntMdINTR);
    InterruptCallback(0, count_interrupt);
    InterruptCallback(0, NULL);
    interrupt_calls = 0;
    Psyz_RcntAdd(44100);
    zexpect_s32_eq(0, interrupt_calls);
}

// SsStart saves the VBLANK handler it replaces and SsEnd puts it back; a
// second SsStart must not find its own handler there.
ZTEST(interrupt_callback, restart_finds_the_restored_handler) {
    zskip_targets("ps1");
    void* saved;

    InterruptCallback(0, NULL);
    SetRCnt(RCntCNT3, 1, RCntMdINTR);
    saved = InterruptCallback(0, NULL);
    InterruptCallback(0, count_interrupt);
    InterruptCallback(0, saved);
    SetRCnt(RCntCNT3, 1, RCntMdINTR);
    zexpect_ptr_eq(NULL, InterruptCallback(0, NULL));
}

ZTEST(interrupt_callback, root_counter_2_is_interrupt_6) {
    zskip_targets("ps1");
    InterruptCallback(6, NULL);
    SetRCnt(RCntCNT2, 44100, RCntMdINTR);
    InterruptCallback(6, count_interrupt);
    interrupt_calls = 0;
    Psyz_RcntAdd(44100);
    zexpect_s32_gt(0, interrupt_calls);
    zexpect_ptr_eq(count_interrupt, InterruptCallback(6, NULL));
}
