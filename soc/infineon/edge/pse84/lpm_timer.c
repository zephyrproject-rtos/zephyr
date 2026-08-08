/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief LPM companion timer for PSE84 using MCWDT.
 *
 * When SysTick is the primary system timer it stops during deepsleep because
 * the HF clocks are gated. This module drives the MCWDT (Multi-Counter
 * Watchdog Timer) from the PILO (32 kHz) clock and implements the
 * z_sys_clock_lpm_enter/exit hooks so the SysTick driver can reconcile elapsed
 * time after wakeup.
 *
 *   - Counter 0 and Counter 1 form a single 32-bit timer through the
 *     match-based C0->C1 cascade: C0 free-runs as the low 16 bits and, each
 *     time it reaches its match value, increments C1 (the high 16 bits).  The
 *     wake alarm is programmed as C0_match = C0 + (delay & 0xFFFF) and
 *     C1_match = C1 + (delay >> 16), giving exact single-tick granularity for
 *     the whole 32-bit range instead of the ~2 s granularity of a roll-over
 *     cascade.
 *
 *   - Counter 1 is the ONLY counter whose interrupt is enabled.  On this IP a
 *     raw MCWDT match wakes the SoC from deepsleep regardless of the interrupt
 *     mask, so Counter 0 is left in NONE mode (it never raises an interrupt and
 *     therefore can never wake the SoC early); only the C1 alarm does.
 *
 *   - Counter 2 free-runs as a 32-bit elapsed-time reference: it is sampled at
 *     entry and read again at exit, and the difference is the time actually
 *     slept.
 *
 * All three counters are started once at init and run continuously.  The MCWDT
 * lives in the always-on LFCLK domain, so the counters keep advancing through
 * both regular DeepSleep and the DeepSleep-RAM warm boot; each LPM entry only
 * reprograms the C0/C1 match and each exit reads C2, without ever stopping or
 * resetting the counters.  That keeps the timing identical across the two sleep
 * modes.
 */

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer_lpm.h>
#include <zephyr/irq.h>

#include <cy_mcwdt.h>

#define DT_DRV_COMPAT infineon_lp_timer

/* Use the first enabled instance - board DTS enables mcwdt0 for M33, mcwdt1 for M55 */
#if !DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#error "No infineon,lp-timer node enabled in devicetree"
#endif

#define MCWDT_NODE     DT_INST(0, DT_DRV_COMPAT)
#define MCWDT_REG_ADDR DT_REG_ADDR(MCWDT_NODE)
#define MCWDT_IRQ_NUM  DT_IRQN(MCWDT_NODE)
#define MCWDT_IRQ_PRIO DT_IRQ(MCWDT_NODE, priority)

/* PILO clock frequency - the MCWDT source clock */
#define PILO_FREQ DT_PROP(DT_PATH(clk_pilo), clock_frequency)

/* Minimum alarm in PILO ticks: MCWDT match values take effect after ~2 PILO
 * cycles, so the shortest deterministic delay is 3 ticks.
 */
#define LPM_MIN_DELAY_TICKS 3U

/* Maximum programmable delay.  Matches the HAL cap: it leaves headroom in the
 * high 16 bits (Counter 1) so the combined match never wraps onto itself.
 */
#define LPM_MAX_DELAY_TICKS 0xFFF0FFFFUL

/* Width of Counter 0 (and Counter 1); each is a 16-bit counter. */
#define LPM_C0_PERIOD 0x10000U

/* Post-write synchronization delay for Cy_MCWDT_SetMatch().  The alarm is armed
 * far from the live count (the cascade-settle loops below guarantee it), so the
 * 2-cycle sync never races the match and no blocking wait is required.
 */
#define MCWDT_SETMATCH_TIME_US 0U

/* Startup delay for Cy_MCWDT_Enable(), in microseconds (~3 LFCLK cycles at
 * 32 kHz), so the counters are guaranteed running before the first alarm.
 */
#define MCWDT_ENABLE_DELAY_US 93U

/* Bound on the cascade-settle busy-wait loops.  One LFCLK cycle is ~30 us at
 * 32 kHz; this many CPU iterations covers the worst-case propagation while
 * still terminating if the LFCLK source ever stops (avoids a lockup).
 */
#define LPM_CASCADE_TIMEOUT 0xFFFFU

static MCWDT_STRUCT_Type *const mcwdt_base = (MCWDT_STRUCT_Type *)MCWDT_REG_ADDR;

/* Counter 0 match value programmed by the previous entry.  Tracked here so the
 * next entry can wait for that queued cascade to settle without reading it back
 * from the hardware.
 */
static uint16_t lpm_c0_last_match = 0xFFFFU;

/* Counter value captured at LPM entry */
static uint32_t lpm_entry_c2;

static void lpm_timer_isr(void)
{
	/* Counter 1 (the alarm) fired.  Clear all match flags and disable the
	 * alarm until the next entry re-arms it.
	 */
	Cy_MCWDT_ClearInterrupt(mcwdt_base, CY_MCWDT_CTR0 | CY_MCWDT_CTR1 | CY_MCWDT_CTR2);
	Cy_MCWDT_SetInterruptMask(mcwdt_base, 0U);
}

void z_sys_clock_lpm_enter(uint64_t max_lpm_time_us)
{
	uint32_t delay_ticks;
	uint16_t c0_now;
	uint16_t c0_match;
	uint16_t c0_settle;
	uint16_t c1_now;
	uint16_t c1_match;
	uint32_t timeout;
	uint32_t key;

	/* Convert microseconds to PILO ticks, clamped to the programmable range. */
	if (max_lpm_time_us > ((uint64_t)LPM_MAX_DELAY_TICKS * 1000000ULL / PILO_FREQ)) {
		delay_ticks = LPM_MAX_DELAY_TICKS;
	} else {
		delay_ticks = (uint32_t)((max_lpm_time_us * PILO_FREQ) / 1000000ULL);
	}

	if (delay_ticks < LPM_MIN_DELAY_TICKS) {
		delay_ticks = LPM_MIN_DELAY_TICKS;
	}
	if (delay_ticks > LPM_MAX_DELAY_TICKS) {
		delay_ticks = LPM_MAX_DELAY_TICKS;
	}

	/* Disarm the alarm while the match registers are reprogrammed. */
	Cy_MCWDT_SetInterruptMask(mcwdt_base, 0U);
	Cy_MCWDT_ClearInterrupt(mcwdt_base, CY_MCWDT_CTR1);

	key = irq_lock();

	/* Wait for any queued C0->C1 cascade from the previous match to settle.
	 * The cascade takes up to ~1 LFCLK cycle after C0 reaches its match and up
	 * to another LFCLK cycle to propagate to the HFCLK-domain registers the CPU
	 * reads, so C0 is read until it is clear of the previous match value.
	 */
	timeout = LPM_CASCADE_TIMEOUT;
	c0_now = (uint16_t)Cy_MCWDT_GetCount(mcwdt_base, CY_MCWDT_COUNTER0);
	while ((((uint16_t)(lpm_c0_last_match - 1U) == c0_now) || (lpm_c0_last_match == c0_now) ||
		((uint16_t)(lpm_c0_last_match + 1U) == c0_now)) &&
	       (timeout != 0U)) {
		c0_now = (uint16_t)Cy_MCWDT_GetCount(mcwdt_base, CY_MCWDT_COUNTER0);
		timeout--;
	}

	/* A new match written within ~2 LFCLK cycles of the live count has
	 * ambiguous first-cascade timing.  Advance past that window and fold the
	 * ticks skipped while waiting back into the remaining delay so the total
	 * stays exact.
	 */
	c0_match = (uint16_t)(c0_now + delay_ticks);
	timeout = LPM_CASCADE_TIMEOUT;
	c0_settle = c0_now;
	while (((c0_settle == c0_match) || (c0_settle == (uint16_t)(c0_match + 1U)) ||
		(c0_settle == (uint16_t)(c0_match + 2U))) &&
	       (timeout != 0U)) {
		c0_settle = (uint16_t)Cy_MCWDT_GetCount(mcwdt_base, CY_MCWDT_COUNTER0);
		timeout--;
	}

	delay_ticks -= (c0_settle >= c0_now) ? (uint32_t)(c0_settle - c0_now)
					     : (uint32_t)((LPM_C0_PERIOD - c0_now) + c0_settle);

	/* Program the 32-bit match: low 16 bits into Counter 0, high 16 bits into
	 * Counter 1.  Counter 1 fires when it is bumped off its match value by the
	 * next Counter 0 match, i.e. after exactly delay_ticks.
	 */
	c0_match = (uint16_t)(c0_now + delay_ticks);
	c1_now = (uint16_t)Cy_MCWDT_GetCount(mcwdt_base, CY_MCWDT_COUNTER1);
	c1_match = (uint16_t)(c1_now + (delay_ticks >> 16));

	Cy_MCWDT_SetMatch(mcwdt_base, CY_MCWDT_COUNTER0, c0_match, MCWDT_SETMATCH_TIME_US);
	Cy_MCWDT_SetMatch(mcwdt_base, CY_MCWDT_COUNTER1, c1_match, MCWDT_SETMATCH_TIME_US);
	lpm_c0_last_match = c0_match;

	/* Snapshot the free-running elapsed reference at the same instant. */
	lpm_entry_c2 = Cy_MCWDT_GetCount(mcwdt_base, CY_MCWDT_COUNTER2);

	irq_unlock(key);

	/* Arm the alarm: Counter 1 is the only counter allowed to wake the SoC. */
	Cy_MCWDT_ClearInterrupt(mcwdt_base, CY_MCWDT_CTR1);
	Cy_MCWDT_SetInterruptMask(mcwdt_base, CY_MCWDT_CTR1);
	irq_enable(MCWDT_IRQ_NUM);
}

uint64_t z_sys_clock_lpm_exit(void)
{
	uint32_t current_count;
	uint32_t elapsed_ticks;

	/* Disable further MCWDT interrupts */
	Cy_MCWDT_SetInterruptMask(mcwdt_base, 0U);
	Cy_MCWDT_ClearInterrupt(mcwdt_base, CY_MCWDT_CTR1);
	irq_disable(MCWDT_IRQ_NUM);

	/* Read the elapsed-time reference (C2 is 32-bit and free-running). */
	current_count = Cy_MCWDT_GetCount(mcwdt_base, CY_MCWDT_COUNTER2);

	/* Unsigned subtraction handles wraparound correctly. */
	elapsed_ticks = current_count - lpm_entry_c2;

	/* Convert PILO ticks to microseconds */
	return ((uint64_t)elapsed_ticks * 1000000ULL) / PILO_FREQ;
}

static int lpm_timer_init(void)
{
	static const cy_stc_mcwdt_config_t cfg = {
		.c0Match = 0xFFFFU,
		.c1Match = 0xFFFFU,
		/* Counter 0 never interrupts (a raw match would wake the SoC); it
		 * only clocks the cascade into Counter 1.
		 */
		.c0Mode = CY_MCWDT_MODE_NONE,
		/* Counter 1 is the wake alarm */
		.c1Mode = CY_MCWDT_MODE_INT,
		/* Counter 2 free-runs as the elapsed-time reference */
		.c2Mode = CY_MCWDT_MODE_NONE,
		.c2ToggleBit = 0U,
		/* Counter 0 free-runs as the cascade prescaler (no clear on match) */
		.c0ClearOnMatch = false,
		.c1ClearOnMatch = false,
		/* Match-based C0->C1 cascade forms the 32-bit alarm timer */
		.c0c1Cascade = true,
		.c1c2Cascade = false,
	};
	cy_rslt_t rslt;

	rslt = (cy_rslt_t)Cy_MCWDT_Init(mcwdt_base, &cfg);
	if (rslt != CY_RSLT_SUCCESS) {
		return -EINVAL;
	}

	/* Start C0/C1/C2 once and leave them running continuously.  The alarm is
	 * kept masked until the first z_sys_clock_lpm_enter() arms it.
	 */
	Cy_MCWDT_SetInterruptMask(mcwdt_base, 0U);
	Cy_MCWDT_Enable(mcwdt_base, CY_MCWDT_CTR0 | CY_MCWDT_CTR1 | CY_MCWDT_CTR2,
			MCWDT_ENABLE_DELAY_US);

	lpm_c0_last_match = 0xFFFFU;

	IRQ_CONNECT(MCWDT_IRQ_NUM, MCWDT_IRQ_PRIO, lpm_timer_isr, NULL, 0);

	return 0;
}

SYS_INIT(lpm_timer_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
