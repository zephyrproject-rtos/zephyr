/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (c) 2018 Xilinx, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_ttcps

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/sys_clock.h>
#include <soc.h>
#include <zephyr/drivers/timer/system_timer.h>
#include "xlnx_psttc_timer_priv.h"

#define TIMER_INDEX		CONFIG_XLNX_PSTTC_TIMER_INDEX

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
#define CYCLES_NEXT_MAX		(XTTC_MAX_INTERVAL_COUNT)

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

	/* Ensure that the match value meets the minimum timing requirements */
	if (delta < CYCLES_NEXT_MIN) {
		match += CYCLES_NEXT_MIN - delta;
	}

	/* Write counter match value for interrupt generation */
	sys_write32(match, TIMER_BASE_ADDR + XTTCPS_MATCH_0_OFFSET);
}

static void ttc_isr(const void *arg)
{
	uint32_t cycles;
	uint32_t ticks;

	ARG_UNUSED(arg);

	/* Acknowledge interrupt */
	sys_read32(TIMER_BASE_ADDR + XTTCPS_ISR_OFFSET);

	/* Read counter value */
	cycles = read_count();

#ifdef CONFIG_TICKLESS_KERNEL
	/* Calculate the number of ticks since last announcement  */
	ticks = (cycles - last_cycles) / CYCLES_PER_TICK;

	/* Update last cycles count */
	last_cycles = cycles;
#else
	/* Update counter match value for the next interrupt */
	update_match(cycles, cycles + CYCLES_PER_TICK);

	/* Advance tick count by 1 */
	ticks = 1;
#endif

	/* Announce to the kernel*/
	sys_clock_announce(ticks);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
#ifdef CONFIG_TICKLESS_KERNEL
	uint32_t cycles;
	uint32_t next_cycles;

	/* Read counter value */
	cycles = read_count();

	/* Calculate timeout counter value */
	if (ticks == K_TICKS_FOREVER) {
		next_cycles = cycles + CYCLES_NEXT_MAX;
	} else {
		next_cycles = cycles + ((uint32_t)ticks * CYCLES_PER_TICK);
	}

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

static int sys_clock_driver_init(const struct device *dev)
{
	uint32_t reg_val;
	ARG_UNUSED(dev);

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
