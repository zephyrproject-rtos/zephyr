/*
 * Copyright (c) 2017, Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>

/*
 * soc_prep_hook runs before arch_bss_zero (BSS has not been zeroed yet,
 * so no global variables may be used here).
 *
 * Two hardware defaults on MSP432P401R create an infinite hard-reset loop
 * when no 32.768 kHz crystal is fitted (as on the LaunchPad board):
 *
 * 1. WDT_A (primary cause): WDT_A powers on in watchdog mode clocked from
 *    SMCLK at 2^15 = 32768 counts (WDTCTL reset value = 0x6904, WDTIS=4).
 *    At the 3 MHz DCO default this times out in ~10.9 ms, which is shorter
 *    than Zephyr's arch_bss_zero pass.  The WDT fires HARDRESET_STAT.SRC1
 *    before the kernel reaches main().
 *
 * 2. CS LFXT fault (secondary, affects NMI on warm resets): ACLK is sourced
 *    from LFXTCLK by default (SELA=0 in CS_CTL1).  Because no crystal is
 *    fitted, LFXTIFG fires and NMI_CTLSTAT.CS_SRC is set on the first warm
 *    reset.  On every subsequent boot LFXTIFG and CS_SRC persist through the
 *    hard reset and re-fire the NMI before the kernel can clear them.
 *
 * Fix strategy:
 *   - soc_early_reset_hook (called before any stack or BSS use): stops WDT_A
 *     immediately and deselects LFXT to prevent a new LFXTIFG fault.
 *   - soc_prep_hook (called later): disables the CS NMI source, switches
 *     ACLK/BCLK permanently to REFO, and clears lingering CS fault flags.
 *
 * Three register states persist through CSS hard reset (only POR clears them):
 *   1. SYSCTL_NMI_CTLSTAT.CS_SRC=1  CS faults routed to ARM NMI
 *   2. CS_IFG.LFXTIFG=1             LFXT fault flag still set
 *   3. CS_IFG.FCNTLFIFG=1           LFXT fault counter flag still set
 *
 * On each new boot after a CSS reset, SYSCTL_NMI_CTLSTAT.CS_SRC=1 and
 * LFXTIFG=1 cause the NMI to re-fire before soc_prep_hook can clear anything,
 * perpetuating the loop.  TI DriverLib (CS_startLFXTWithTimeout) follows the
 * same pattern: it saves and disables the CS NMI source before any CS work,
 * then re-enables it after the fault flags are cleared.
 *
 * Fix sequence (mirrors TI DriverLib approach):
 *   1. Disable CS NMI source in SYSCTL_NMI_CTLSTAT -- prevents NMI firing
 *      while fault flags are still set.
 *   2. Unlock CS; disable and reset the LFXT fault counter (CTL3) -- stops
 *      FCNTLF before it can fire again.
 *   3. Switch ACLK (SELA) and BCLK (SELB) from LFXTCLK to REFOCLK -- REFO
 *      is always available without an external crystal.
 *   4. Wait for ACLK_READY and BCLK_READY, then for LFXT_ON to go low.
 *   5. Clear all CS fault flags via CLRIFG; lock CS.
 *
 * SystemInit() (called later from soc_early_init_hook) configures MCLK/SMCLK
 * for the target frequency via DCO and does not touch ACLK or BCLK, so these
 * settings are preserved through full boot.
 */

/*
 * soc_early_reset_hook runs from the ARM reset handler before any stack
 * setup or RAM access.  It must use no stack.  On Cortex-M the return
 * address is in LR and MSP is valid (loaded from vector table slot 0), so
 * a plain C function with no locals compiles cleanly without push/pop.
 *
 * Timing (3 MHz DCO, ~50 instructions):
 *   This hook runs at roughly cycle 50 from POR/CSS-reset.
 *   LFXTIFG fires at cycle ~92 (one LFXT oscillation-monitor period).
 *   Clearing FCNTLF_EN here prevents the fault counter from starting when
 *   LFXTIFG rises, which is the only reliable way to stop the counter: once
 *   the LFXTIFG 0→1 edge has occurred with FCNTLF_EN=1, the 32768-cycle
 *   countdown cannot be aborted by any subsequent register write.
 */
void soc_early_reset_hook(void)
{
	/*
	 * WDT_A powers on in watchdog mode clocked by SMCLK at 2^15 counts
	 * (WDTIS=4, the hardware default), which times out at ~10.9 ms at the
	 * 3 MHz DCO default.  That is shorter than the time Zephyr takes to
	 * complete arch_bss_zero, so the watchdog fires HARDRESET_STAT.SRC1
	 * before the kernel reaches main.  Stop it immediately.
	 */
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;

	SYSCTL->NMI_CTLSTAT &= ~SYSCTL_NMI_CTLSTAT_CS_SRC;

	CS->KEY = CS_KEY_VAL;

	/*
	 * Switch ACLK (SELA) and BCLK (SELB) from LFXT to REFO at the
	 * earliest possible moment (~cycle 20 from POR/reset).  The CSS
	 * fault counter starts when LFXT is the SELECTED source for ACLK
	 * or BCLK and fails to oscillate.  TI's functional description
	 * states the Hard Reset fires only when "ACLK or BCLK is sourced
	 * from a faulty oscillator."  Deselecting LFXT here, before the
	 * oscillation monitor's first check period (~cycle 92), eliminates
	 * the fault condition before it can be detected.
	 *
	 * After a CSS hard reset, CS_CTL1 returns to its POR default
	 * (SELA=0=LFXT, SELB=0=LFXT), so this write is needed on every
	 * boot, not just after POR.
	 */
	CS->CTL1 = (CS->CTL1 & ~(CS_CTL1_SELA_MASK | CS_CTL1_SELB)) |
		   CS_CTL1_SELA__REFOCLK | CS_CTL1_SELB;

	/* Belt-and-suspenders: also reset and disable the fault counter. */
	CS->CTL3 |= CS_CTL3_RFCNTLF;
	CS->CTL3 &= ~CS_CTL3_FCNTLF_EN;

	CS->KEY = 0;
}

void soc_prep_hook(void)
{
	/*
	 * Disable CS fault NMI source before any CS manipulation.
	 * SYSCTL_NMI_CTLSTAT.CS_SRC persists through CSS hard resets, so it
	 * is already 1 on every boot after the first LFXT fault.  Leaving it
	 * enabled while LFXTIFG is still set would immediately re-fire the NMI
	 * and re-trigger a CSS hard reset before we can clear the flags.
	 * We do not re-enable it; with no LFXT crystal fitted, CS NMI serves
	 * no useful purpose and would only re-introduce the reset loop.
	 */
	SYSCTL->NMI_CTLSTAT &= ~SYSCTL_NMI_CTLSTAT_CS_SRC;

	/* Unlock CS module registers. */
	CS->KEY = CS_KEY_VAL;

	/*
	 * Disable the LFXT start fault counter before switching clock sources.
	 * The counter starts when LFXTIFG is set and fires (HARDRESET) after
	 * 32768 MCLK cycles (~10.9 ms at 3 MHz DCO).
	 *
	 * RFCNTLF must be written as a separate write BEFORE clearing
	 * FCNTLF_EN.  The hardware ignores RFCNTLF when FCNTLF_EN is being
	 * cleared in the same write (mirrors TI DriverLib which issues
	 * CS_resetFaultCounter and CS_disableFaultCounter as two separate
	 * single-bit writes).
	 */
	CS->CTL3 |= CS_CTL3_RFCNTLF;        /* reset counter (EN still = 1) */
	CS->CTL3 &= ~CS_CTL3_FCNTLF_EN;     /* then disable it              */

	/*
	 * Switch ACLK (SELA, bits 10:8) and BCLK (SELB, bit 12) from
	 * LFXTCLK (reset default, both=0) to REFOCLK.
	 * Leave SELM and SELS unchanged (DCO at reset state).
	 */
	CS->CTL1 = (CS->CTL1 & ~(CS_CTL1_SELA_MASK | CS_CTL1_SELB)) |
		   CS_CTL1_SELA__REFOCLK | CS_CTL1_SELB;

	/*
	 * Wait for ACLK and BCLK to switch to REFOCLK.  Until ACLK_READY
	 * and BCLK_READY are both set, LFXT is still considered "selected"
	 * by the hardware and LFXTIFG cannot be cleared.  Guard count prevents
	 * an infinite spin on defective hardware.
	 */
	for (int guard = 10000;
	     guard > 0 &&
	     (CS->STAT & (CS_STAT_ACLK_READY | CS_STAT_BCLK_READY)) !=
	     (CS_STAT_ACLK_READY | CS_STAT_BCLK_READY);
	     guard--) {
	}

	/* Wait for LFXT oscillator to stop (LFXT_ON goes low). */
	for (int guard = 10000;
	     guard > 0 && (CS->STAT & CS_STAT_LFXT_ON);
	     guard--) {
	}

	/*
	 * Clear all CS fault flags now that LFXT is off and the NMI source is
	 * disabled.  LFXTIFG and FCNTLFIFG persist through CSS hard resets;
	 * clearing them here prevents RSTCTL from re-triggering on the next
	 * boot if the chip is reset by other means.
	 */
	CS->CLRIFG = CS_CLRIFG_CLR_LFXTIFG | CS_CLRIFG_CLR_HFXTIFG |
		     CS_CLRIFG_CLR_FCNTLFIFG | CS_CLRIFG_CLR_FCNTHFIFG;

	/* Lock CS module registers. */
	CS->KEY = 0;
}

void soc_early_init_hook(void)
{
	SystemInit();
}
