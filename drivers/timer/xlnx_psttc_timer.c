/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (c) 2018 Xilinx, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_ttcps

#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/sys/clock.h>
#include <soc.h>
#include <zephyr/drivers/timer/system_timer.h>
#include "xlnx_psttc_timer_priv.h"

#define TIMER_IRQ		DT_INST_IRQN(0)
#define TIMER_BASE_ADDR		DT_INST_REG_ADDR(0)
#define TIMER_CLOCK_FREQUECY	DT_INST_PROP(0, clock_frequency)

#define TICKS_PER_SEC		CONFIG_SYS_CLOCK_TICKS_PER_SEC
#define CYCLES_PER_SEC		TIMER_CLOCK_FREQUECY
#define CYCLES_PER_TICK		(CYCLES_PER_SEC / TICKS_PER_SEC)

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = DT_IRQN(DT_INST(0, xlnx_ttcps));
#endif
/*
 * CYCLES_NEXT_MIN must be large enough to ensure that the timer does not miss
 * interrupts.  This value was conservatively set using the trial and error
 * method, and there is room for improvement.
 */
#define CYCLES_NEXT_MIN		(10000)
/*
 * update_match() also re-arms when the counter read back after the write is
 * less than CYCLES_READBACK_MARGIN cycles short of the match: the counter
 * could step onto a match that close before the new value reaches the
 * comparator.
 */
#define CYCLES_READBACK_MARGIN	(100U)
#define CYCLES_NEXT_MAX		(XTTC_MAX_INTERVAL_COUNT)
#define MAX_TICKS		((XTTC_MAX_INTERVAL_COUNT / 4U) / CYCLES_PER_TICK)

BUILD_ASSERT(TIMER_CLOCK_FREQUECY ==
			CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
	     "Configured system timer frequency does not match the TTC "
	     "clock frequency in the device tree");

BUILD_ASSERT(CYCLES_PER_SEC >= TICKS_PER_SEC,
	     "Timer clock frequency must be greater than the system tick "
	     "frequency");

BUILD_ASSERT((CYCLES_PER_SEC % TICKS_PER_SEC) == 0,
	     "Timer clock frequency is not divisible by the system tick "
	     "frequency");

#ifdef CONFIG_TICKLESS_KERNEL
static uint32_t last_cycles;
#endif

static uint32_t read_count(void)
{
	/* Read current counter value */
	return sys_read32(TIMER_BASE_ADDR + XTTCPS_COUNT_VALUE_OFFSET);
}

static void update_match(uint32_t cycles, uint32_t match)
{
	uint32_t delta = match - cycles;
	uint32_t now;

	/* Ensure that the match value meets the minimum timing requirements */
	if (delta < CYCLES_NEXT_MIN) {
		match += CYCLES_NEXT_MIN - delta;
	}

	/* Write counter match value for interrupt generation */
	sys_write32(match, TIMER_BASE_ADDR + XTTCPS_MATCH_0_OFFSET);

	/* The match interrupt fires only when the counter steps onto the match
	 * value. If anything held this path up for CYCLES_NEXT_MIN cycles, the
	 * counter can already be at or past the value just written, and the next
	 * timer interrupt would come only after the 32-bit counter wraps (42.9 s
	 * at 100 MHz). Read the counter back after the write and move the match
	 * ahead until it is at least CYCLES_READBACK_MARGIN cycles in front of
	 * the counter.
	 */
	now = read_count();
	while ((now - cycles) >= ((match - cycles) - CYCLES_READBACK_MARGIN)) {
		cycles = now;
		match = now + CYCLES_NEXT_MIN;
		sys_write32(match, TIMER_BASE_ADDR + XTTCPS_MATCH_0_OFFSET);
		now = read_count();
	}
}

static void ttc_isr(const void *arg)
{
	k_spinlock_key_t key;
	uint32_t cycles;
	uint32_t ticks;

	ARG_UNUSED(arg);

	/* Acknowledge interrupt */
	sys_read32(TIMER_BASE_ADDR + XTTCPS_ISR_OFFSET);

	/*
	 * Hold the system clock lock from reading the counter until the
	 * kernel has taken the ticks. A timeout started by an interrupt
	 * that preempts this ISR after last_cycles has moved, but before
	 * the ticks are announced, would be timed from a curr_tick short
	 * by those ticks and expire early.
	 */
	key = sys_clock_lock();

	/* Read counter value */
	cycles = read_count();

#ifdef CONFIG_TICKLESS_KERNEL
	/* Calculate the number of ticks since last announcement  */
	ticks = (cycles - last_cycles) / CYCLES_PER_TICK;

	/* Update last cycles count */
	last_cycles += ticks * CYCLES_PER_TICK;
#else
	/* Update counter match value for the next interrupt */
	update_match(cycles, cycles + CYCLES_PER_TICK);

	/* Advance tick count by 1 */
	ticks = 1;
#endif

	/* Announce to the kernel, which releases the lock */
	sys_clock_announce_locked(ticks, key);
}

void sys_clock_set_timeout(uint32_t ticks, bool idle)
{
#ifdef CONFIG_TICKLESS_KERNEL
	uint32_t cycles;
	uint32_t next_cycles;
	uint32_t elapsed;

	/* Read counter value */
	cycles = read_count();

	/* Clamp so the next announcement is at most MAX_TICKS after the last
	 * one, not after now: when every timeout is re-armed before it
	 * expires, the match must still be hit before the 32-bit distance
	 * from last_cycles wraps (42.9 s), or elapsed time is lost.
	 */
	elapsed = (cycles - last_cycles) / CYCLES_PER_TICK;
	ticks = MAX(ticks, 1U);
	if (elapsed >= MAX_TICKS) {
		ticks = 1U;
	} else if (ticks > (MAX_TICKS - elapsed)) {
		ticks = MAX_TICKS - elapsed;
	}

	/* Next announcement on a tick boundary counted from the last one */
	next_cycles = last_cycles + (elapsed + ticks) * CYCLES_PER_TICK;

	/* Set match value for the next interrupt */
	update_match(cycles, next_cycles);
#endif
}

uint32_t sys_clock_elapsed(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	uint32_t cycles;

	/* Read counter value */
	cycles = read_count();

	/* Return the number of ticks since last announcement */
	return (cycles - last_cycles) / CYCLES_PER_TICK;
#else
	/* Always return 0 for tickful operation */
	return 0;
#endif
}

uint32_t sys_clock_cycle_get_32(void)
{
	/* Return the current counter value */
	return read_count();
}

static int sys_clock_driver_init(void)
{
	uint32_t reg_val;

	/* Stop timer */
	sys_write32(XTTCPS_CNT_CNTRL_DIS_MASK,
		    TIMER_BASE_ADDR + XTTCPS_CNT_CNTRL_OFFSET);

#ifdef CONFIG_TICKLESS_KERNEL
	/* Initialise internal states */
	last_cycles = 0;
#endif

	/* Initialise timer registers */
	sys_write32(XTTCPS_CNT_CNTRL_RESET_VALUE,
		    TIMER_BASE_ADDR + XTTCPS_CNT_CNTRL_OFFSET);
	sys_write32(0, TIMER_BASE_ADDR + XTTCPS_CLK_CNTRL_OFFSET);
	sys_write32(0, TIMER_BASE_ADDR + XTTCPS_INTERVAL_VAL_OFFSET);
	sys_write32(0, TIMER_BASE_ADDR + XTTCPS_MATCH_0_OFFSET);
	sys_write32(0, TIMER_BASE_ADDR + XTTCPS_MATCH_1_OFFSET);
	sys_write32(0, TIMER_BASE_ADDR + XTTCPS_MATCH_2_OFFSET);
	sys_write32(0, TIMER_BASE_ADDR + XTTCPS_IER_OFFSET);
	sys_write32(XTTCPS_IXR_ALL_MASK, TIMER_BASE_ADDR + XTTCPS_ISR_OFFSET);
	/*
	 * The interrupt register clears on read, as the ISR relies on. The
	 * write above does not clear a match latched before init, which would
	 * raise one timer interrupt as soon as the interrupt is enabled.
	 */
	(void)sys_read32(TIMER_BASE_ADDR + XTTCPS_ISR_OFFSET);

	/* Reset counter value */
	reg_val = sys_read32(TIMER_BASE_ADDR + XTTCPS_CNT_CNTRL_OFFSET);
	reg_val |= XTTCPS_CNT_CNTRL_RST_MASK;
	sys_write32(reg_val, TIMER_BASE_ADDR + XTTCPS_CNT_CNTRL_OFFSET);

	/* Set match mode */
	reg_val = sys_read32(TIMER_BASE_ADDR + XTTCPS_CNT_CNTRL_OFFSET);
	reg_val |= XTTCPS_CNT_CNTRL_MATCH_MASK;
	sys_write32(reg_val, TIMER_BASE_ADDR + XTTCPS_CNT_CNTRL_OFFSET);

	/* Set initial timeout */
	reg_val = IS_ENABLED(CONFIG_TICKLESS_KERNEL) ?
			CYCLES_NEXT_MAX : CYCLES_PER_TICK;
	sys_write32(reg_val, TIMER_BASE_ADDR + XTTCPS_MATCH_0_OFFSET);

	/* Connect timer interrupt */
	IRQ_CONNECT(TIMER_IRQ, 0, ttc_isr, 0, 0);
	irq_enable(TIMER_IRQ);

	/* Enable timer interrupt */
	reg_val = sys_read32(TIMER_BASE_ADDR + XTTCPS_IER_OFFSET);
	reg_val |= XTTCPS_IXR_MATCH_0_MASK;
	sys_write32(reg_val, TIMER_BASE_ADDR + XTTCPS_IER_OFFSET);

	/* Start timer */
	reg_val = sys_read32(TIMER_BASE_ADDR + XTTCPS_CNT_CNTRL_OFFSET);
	reg_val &= (~XTTCPS_CNT_CNTRL_DIS_MASK);
	sys_write32(reg_val, TIMER_BASE_ADDR + XTTCPS_CNT_CNTRL_OFFSET);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
