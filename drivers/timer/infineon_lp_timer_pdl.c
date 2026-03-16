/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Low Power timer driver for Infineon CAT1 MCU family.
 */

#define DT_DRV_COMPAT infineon_lp_timer

#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys_clock.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ifx_cat1_lp_timer_pdl, CONFIG_KERNEL_LOG_LEVEL);

/* Enable the LPTimer counters.  Here we enable two 16-bit counters and one 32-bit counter to
 * create a 64-bit counter
 */
#define LPTIMER_COUNTERS (CY_MCWDT_CTR0 | CY_MCWDT_CTR1 | CY_MCWDT_CTR2)

/* The application only needs one lptimer. Report an error if more than one is selected. */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1
#error Only one LPTIMER instance should be enabled
#endif /* DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1 */

/* Minimum amount of lfclk cycles of that LPTIMER can delay for. */
#define LPTIMER_MIN_DELAY       (3U)
/* ~36hours. Not set to 0xffffffff to avoid C0 and C1 both overflowing */
#define LPTIMER_MAX_DELAY_TICKS (0xfff0ffffUL)

static bool clear_int_mask;
static uint8_t isr_instruction;

static MCWDT_STRUCT_Type *reg_addr = (MCWDT_STRUCT_Type *)DT_INST_REG_ADDR(0);
static const uint32_t clock_frequency = DT_INST_PROP(0, clock_frequency);

#define DEFAULT_TIMEOUT (0xFFFFUL)

#include "cy_mcwdt.h"

#if defined(CY_IP_MXS40SSRSS)
static const uint16_t LPTIMER_RESET_TIME_US = 93;
#else
static const uint16_t LPTIMER_RESET_TIME_US = 62;
#endif

/* The value of this variable is intended to be 0 */
static const uint16_t LPTIMER_SETMATCH_TIME_US;

static const cy_stc_mcwdt_config_t lptimer_default_cfg = {.c0Match = 0xFFFF,
							  .c1Match = 0xFFFF,
							  .c0Mode = CY_MCWDT_MODE_INT,
							  .c1Mode = CY_MCWDT_MODE_INT,
							  .c2Mode = CY_MCWDT_MODE_NONE,
							  .c2ToggleBit = 0,
							  .c0ClearOnMatch = false,
							  .c1ClearOnMatch = false,
							  .c0c1Cascade = true,
							  .c1c2Cascade = false};

static uint64_t last_lptimer_value;
static struct k_spinlock lock;

static void lptimer_enable_event(bool enable)
{
#define LPTIMER_ISR_CALL_USER_CB_MASK (0x01)
	isr_instruction &= ~LPTIMER_ISR_CALL_USER_CB_MASK;
	isr_instruction |= (uint8_t)enable;

	if (enable) {
		Cy_MCWDT_ClearInterrupt(reg_addr, CY_MCWDT_CTR1);
		Cy_MCWDT_SetInterruptMask(reg_addr, CY_MCWDT_CTR1);

	} else {
		Cy_MCWDT_ClearInterrupt(reg_addr, CY_MCWDT_CTR1);
		Cy_MCWDT_SetInterruptMask(reg_addr, 0);
	}
}

static void lptimer_set_delay(uint32_t delay)
{
	uint16_t c0_old_match;
	unsigned int key;
	uint32_t timeout = DEFAULT_TIMEOUT;
	uint16_t c0_current_ticks;
	uint16_t c0_match;
	uint32_t c0_new_ticks;
	uint16_t c1_current_ticks;
	uint16_t c1_match;

	clear_int_mask = true;

	if ((Cy_MCWDT_GetEnabledStatus(reg_addr, CY_MCWDT_COUNTER0) == 0UL) ||
	    (Cy_MCWDT_GetEnabledStatus(reg_addr, CY_MCWDT_COUNTER1) == 0UL) ||
	    (Cy_MCWDT_GetEnabledStatus(reg_addr, CY_MCWDT_COUNTER2) == 0UL)) {
		return;
	}

	/* - 16 bit Counter0 (C0) & Counter1 (C1) are cascaded to generated a 32 bit counter.
	 * - Counter2 (C2) is a free running counter.
	 * - C0 continues counting after reaching its match value. On PSoC™ 4 Counter1 is reset on
	 * match. On PSoC™ 6 it continues counting.
	 * - An interrupt is generated when C1 reaches the match value. On PSoC™ 4 this happens
	 * when the counter increments to the same value as match. On PSoC™ 6 this happens when it
	 * increments past the match value.
	 *
	 * EXAMPLE:
	 * Supposed T=C0=C1=0, and we need to trigger an interrupt at T=0x18000.
	 * We set C0_match to 0x8000 and C1 match to 1.
	 * At T = 0x8000, C0_value matches C0_match so C1 get incremented. C1/C0=0x18000.
	 * At T = 0x18000, C0_value matches C0_match again so C1 get incremented from 1 to 2.
	 * When C1 get incremented from 1 to 2 the interrupt is generated.
	 * At T = 0x18000, C1/C0 = 0x28000.
	 */

	if (delay <= LPTIMER_MIN_DELAY) {
		delay = LPTIMER_MIN_DELAY;
	}
	if (delay > LPTIMER_MAX_DELAY_TICKS) {
		delay = LPTIMER_MAX_DELAY_TICKS;
	}

	Cy_MCWDT_ClearInterrupt(reg_addr, CY_MCWDT_CTR1);
	c0_old_match = (uint16_t)Cy_MCWDT_GetMatch(reg_addr, CY_MCWDT_COUNTER0);
	key = irq_lock();

	/* Cascading from C0 match into C1 is queued and can take 1 full LF clk cycle.
	 * There are 3 cases:
	 * Case 1: if c0 = match0 then the cascade into C1 will happen 1 cycle from now. The value
	 * c1_current_ticks is 1 lower than expected. Case 2: if c0 = match0 -1 then cascade may or
	 * not happen before new match value would occur. Match occurs on rising clock edge.
	 *          Synchronizing match value occurs on falling edge. Wait until c0 = match0 to
	 * ensure cascade occurs. Case 3: everything works as expected.
	 *
	 * Note: timeout is needed here just in case the LFCLK source gives out. This avoids device
	 * lockup.
	 *
	 * ((2 * Cycles_LFClk) / Cycles_cpu_iteration) * (HFCLk_max / LFClk_min) =
	 * Iterations_required Typical case: (2 / 100) * ((150x10^6)/33576) = 89 iterations Worst
	 * case: (2 / 100) * ((150x10^6)/1) = 3x10^6 iterations Compromise: (2 / 100) *
	 * ((150x10^6)/0xFFFF iterations) = 45 Hz = LFClk_min
	 */
	c0_current_ticks = (uint16_t)Cy_MCWDT_GetCount(reg_addr, CY_MCWDT_COUNTER0);
	/* Wait until the cascade has definitively happened. It takes a clock cycle for the
	 *   cascade to happen, and potentially another a full LFCLK clock cycle for the
	 *   cascade to propagate up to the HFCLK-domain registers that the CPU reads.
	 */
	while (((((uint16_t)(c0_old_match - 1)) == c0_current_ticks) ||
		(c0_old_match == c0_current_ticks) ||
		(((uint16_t)(c0_old_match + 1)) == c0_current_ticks)) &&
	       (timeout != 0UL)) {
		c0_current_ticks = (uint16_t)Cy_MCWDT_GetCount(reg_addr, CY_MCWDT_COUNTER0);
		timeout--;
	}

	if (timeout == 0UL) {
		/* Timeout has occurred. There could have been a clock failure while waiting for the
		 * count value to update.
		 */
		irq_unlock(key);
		return;
	}

	c0_match = (uint16_t)(c0_current_ticks + delay);
	/* Changes can take up to 2 clk_lf cycles to propagate. If we set the match within this
	 * window of the current value, then it is nondeterministic whether the first cascade will
	 * trigger immediately or after 2^16 cycles. Wait until c0 is in a more predictable state.
	 */
	timeout = DEFAULT_TIMEOUT;
	c0_new_ticks = c0_current_ticks;

	while (((c0_new_ticks == c0_match) || (c0_new_ticks == ((uint16_t)(c0_match + 1))) ||
		(c0_new_ticks == ((uint16_t)(c0_match + 2)))) &&
	       (timeout != 0UL)) {
		c0_new_ticks = (uint16_t)Cy_MCWDT_GetCount(reg_addr, CY_MCWDT_COUNTER0);
		timeout--;
	}

	delay -= (c0_new_ticks >= c0_current_ticks)
			 ? (uint32_t)(c0_new_ticks - c0_current_ticks)
			 : (uint32_t)((0xFFFFU - c0_current_ticks) + c0_new_ticks);

	c0_match = (uint16_t)(c0_current_ticks + delay);
	c1_current_ticks = (uint16_t)Cy_MCWDT_GetCount(reg_addr, CY_MCWDT_COUNTER1);
	c1_match = (uint16_t)(c1_current_ticks + (delay >> 16));

	Cy_MCWDT_SetMatch(reg_addr, CY_MCWDT_COUNTER0, c0_match, LPTIMER_SETMATCH_TIME_US);
	Cy_MCWDT_SetMatch(reg_addr, CY_MCWDT_COUNTER1, c1_match, LPTIMER_SETMATCH_TIME_US);

	irq_unlock(key);
	Cy_MCWDT_SetInterruptMask(reg_addr, CY_MCWDT_CTR1);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	uint64_t current_cycles;
	uint32_t cycles_per_tick;
	uint32_t delay_cycles;
	uint64_t next_tick_cycles;
	k_spinlock_key_t key;

	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	if (ticks == K_TICKS_FOREVER) {
		/* Disable the LPTIMER events */
		lptimer_enable_event(false);
		return;
	}

	/* Configure and Enable the LPTIMER events */
	lptimer_enable_event(true);

	/* passing ticks==1 means "announce the next tick", ticks value of zero (or even negative)
	 * is legal and treated identically: it simply indicates the kernel would like the next
	 * tick announcement as soon as possible.
	 */
	if (ticks < 1) {
		ticks = 1;
	}

	/* Calculate cycles per tick */
	cycles_per_tick = ((clock_frequency / CONFIG_SYS_CLOCK_TICKS_PER_SEC));

	/* Get current cycle count from the free-running counter */
	key = k_spin_lock(&lock);
	current_cycles = Cy_MCWDT_GetCount(reg_addr, CY_MCWDT_COUNTER2);

	/* Calculate the next tick-aligned cycle count that is at least 'ticks' in the future.
	 * This ensures: next_tick_cycles % cycles_per_tick == 0 (tick-aligned)
	 * AND: next_tick_cycles >= current_cycles + ticks * cycles_per_tick
	 */
	next_tick_cycles = ((current_cycles / cycles_per_tick) + ticks) * cycles_per_tick;

	/* Verify we're at least ticks * cycles_per_tick in the future.
	 * Due to integer division, we may be slightly less if not at a tick boundary.
	 * Add one more tick period if needed to satisfy the invariant.
	 */
	if (next_tick_cycles < current_cycles + (ticks * cycles_per_tick)) {
		next_tick_cycles += cycles_per_tick;
	}

	/* Calculate delay from current position to next tick-aligned position.
	 * Unsigned arithmetic handles rollover correctly.
	 */
	delay_cycles = next_tick_cycles - current_cycles;

	/* Ensure minimum delay requirement and check for excessive delays.
	 * The hardware requires a minimum delay and has a maximum delay constraint.
	 */
	if (delay_cycles < LPTIMER_MIN_DELAY) {
		/* If calculated delay is too short, move to next tick boundary */
		next_tick_cycles += cycles_per_tick;
		delay_cycles = next_tick_cycles - current_cycles;
	}

	if (delay_cycles > LPTIMER_MAX_DELAY_TICKS) {
		/* Delay exceeds maximum supported by hardware, clamp to maximum */
		delay_cycles = LPTIMER_MAX_DELAY_TICKS;
	}

	/* Set the delay value for the next wakeup interrupt */
	lptimer_set_delay(delay_cycles);
	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	uint32_t current_cycles;
	uint32_t cycles_per_tick;
	uint32_t delta_cycles;
	uint32_t delta_ticks;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	cycles_per_tick = clock_frequency / CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	k_spinlock_key_t key = k_spin_lock(&lock);

	current_cycles = Cy_MCWDT_GetCount(reg_addr, CY_MCWDT_COUNTER2);

	/* Calculate elapsed hardware cycles since the last announcement */
	delta_cycles = current_cycles - (uint32_t)last_lptimer_value;

	k_spin_unlock(&lock, key);

	/* Convert hardware cycles to kernel ticks */
	delta_ticks = delta_cycles / cycles_per_tick;

	return delta_ticks;
}

uint32_t sys_clock_cycle_get_32(void)
{
	uint32_t cycles;

	/* Read the current hardware cycle count from free-running counter */
	k_spinlock_key_t key = k_spin_lock(&lock);

	cycles = Cy_MCWDT_GetCount(reg_addr, CY_MCWDT_COUNTER2);

	k_spin_unlock(&lock, key);

	return cycles;
}

static void lptimer_isr(void)
{
	Cy_MCWDT_ClearInterrupt(reg_addr, LPTIMER_COUNTERS);

	/* Clear interrupt mask if set only from lptimer_set_delay() function */
	if (clear_int_mask) {
		Cy_MCWDT_SetInterruptMask(reg_addr, 0);
	}

	if ((isr_instruction & LPTIMER_ISR_CALL_USER_CB_MASK) != 0) {
		/* Announce the number of ticks that have elapsed since the last announcement */
		uint32_t current_cycles = Cy_MCWDT_GetCount(reg_addr, CY_MCWDT_COUNTER2);
		uint32_t cycles_per_tick = clock_frequency / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
		uint64_t delta_ticks;
		k_spinlock_key_t key = k_spin_lock(&lock);

		delta_ticks = (uint64_t)((current_cycles - last_lptimer_value) / cycles_per_tick);

		/* Update last announced position to maintain tick alignment */
		last_lptimer_value += delta_ticks * cycles_per_tick;
		k_spin_unlock(&lock, key);

		sys_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? delta_ticks
								      : (delta_ticks > 0));
	}
}

static int lptimer_init(void)
{
	cy_rslt_t rslt = CY_MCWDT_BAD_PARAM;
	cy_stc_mcwdt_config_t cfg = lptimer_default_cfg;

	clear_int_mask = false;
	isr_instruction = LPTIMER_ISR_CALL_USER_CB_MASK;

	rslt = (cy_rslt_t)Cy_MCWDT_Init(reg_addr, &cfg);
	if (rslt == CY_RSLT_SUCCESS) {
		Cy_MCWDT_Enable(reg_addr, LPTIMER_COUNTERS, LPTIMER_RESET_TIME_US);
	} else {
		Cy_MCWDT_Disable(reg_addr, LPTIMER_COUNTERS, LPTIMER_RESET_TIME_US);
		Cy_MCWDT_DeInit(reg_addr);
		return -EINVAL;
	}

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), lptimer_isr, NULL, 0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

SYS_INIT(lptimer_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
