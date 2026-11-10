/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 *
 * A TIMER block of the Raspberry Pi Pico as the system timer, on Zephyr's
 * generic timer core.
 *
 * The block is a 64-bit count of microseconds with four alarms. An alarm
 * holds 32 bits and fires when the low half of the count is equal to it, so
 * the driver works on the low half alone and tells the core that the match
 * is exact: a value written too late is missed until the count wraps.
 */

#define DT_DRV_COMPAT raspberrypi_pico_system_timer

#include <zephyr/drivers/reset.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>

#include <hardware/structs/timer.h>

#define TIMER      ((timer_hw_t *)DT_INST_REG_ADDR(0))
#define ALARM      0
#define TIMER_IRQN DT_INST_IRQ_BY_IDX(0, ALARM, irq)

BUILD_ASSERT(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == DT_INST_PROP(0, clock_frequency),
	     "SYS_CLOCK_HW_CYCLES_PER_SEC must match the TIMER clock-frequency");

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = TIMER_IRQN;
#endif

#define TIMER_CORE_BACKEND_COMPARE_EXACT
#define TIMER_CORE_COUNTER_WIDTH 32

static inline uint32_t timer_driver_cycle_get(void)
{
	return TIMER->timerawl;
}

/* Writing the alarm arms it. */
static inline void timer_driver_set_compare(uint32_t cycles)
{
	TIMER->alarm[ALARM] = cycles;
}

#include "system_timer_generic.h"

static void timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	TIMER->intr = BIT(ALARM);
	timer_core_announce();
}

#ifdef CONFIG_SMP
void smp_timer_init(void)
{
	/* One timer for both cores, and only core 0 takes its interrupt. */
}
#endif

static int sys_clock_driver_init(void)
{
	const struct reset_dt_spec reset = RESET_DT_SPEC_INST_GET(0);
	int ret;

	if (!device_is_ready(reset.dev)) {
		return -ENODEV;
	}

	ret = reset_line_toggle_dt(&reset);
	if (ret < 0) {
		return ret;
	}

	IRQ_CONNECT(TIMER_IRQN, DT_INST_IRQ_BY_IDX(0, ALARM, priority), timer_isr, NULL, 0);

	TIMER->intr = BIT(ALARM);
	TIMER->inte = BIT(ALARM);

	timer_core_init();

	irq_enable(TIMER_IRQN);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
