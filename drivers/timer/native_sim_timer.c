/*
 * Copyright (c) 2017-2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Driver for the native_sim board timer
 * It provides the same API required by the kernel as any other timer driver
 */
#include <zephyr/types.h>
#include <zephyr/irq.h>
#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>
#include "nsi_hw_scheduler.h"
#include "nsi_timer_model.h"
#include "soc.h"

/**
 * Return the current HW cycle counter. This corresponds to the number of
 * microseconds since boot, in 64 bits.
 */
static uint64_t timer_driver_cycle_get(void)
{
	return nsi_hws_get_time();
}

/* Microseconds per kernel tick, the core's TIMER_CORE_CYC_PER_TICK, needed here
 * before the core is included.
 */
#define NP_TICK_PERIOD_US (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/**
 * Program the next tick interrupt <cycles> microseconds from now, by passing over
 * the tick expiries in between.
 *
 * Stretching the model's tick period with hwtimer_enable() instead would let it
 * reach the deadline in a single step, which leaves CONFIG_NATIVE_SIM_SLOWDOWN_TO_REAL_TIME
 * with nothing to pace simulated time against: that runs once per tick expiry.
 *
 * Skipping lands on the model's tick grid, so this can fire up to a tick early.
 * The core announces what actually elapsed and the kernel re-arms.
 */
static void timer_driver_set_reload(uint64_t cycles)
{
	int64_t silent_ticks = (int64_t)(cycles / NP_TICK_PERIOD_US) - 1;

	hwtimer_set_silent_ticks(MAX(silent_ticks, 0));
}

/*
 * Knobs for system_timer_generic.h
 */
#define TIMER_CORE_BACKEND_RELOAD
#define TIMER_CORE_COUNTER_WIDTH 64
#define TIMER_CORE_ALARM_MAX_CYCLES NSI_NEVER

#include "system_timer_generic.h"

BUILD_ASSERT(NP_TICK_PERIOD_US == TIMER_CORE_CYC_PER_TICK,
	     "the tick period used to arm the model must match the core's");

/**
 * Interrupt handler for the timer interrupt
 * Announce to the kernel that a number of ticks have passed
 */
static void np_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	timer_core_announce();
}

/**
 * This function exists only to enable tests to call into the timer ISR
 */
void np_timer_isr_test_hook(const void *arg)
{
	np_timer_isr(NULL);
}

/**
 * @brief Stop announcing sys ticks into the kernel
 *
 * Disable the system ticks generation
 */
void sys_clock_disable(void)
{
	irq_disable(TIMER_TICK_IRQ);
	hwtimer_set_silent_ticks(INT64_MAX);
}

/**
 * @brief Initialize system timer driver
 *
 * Enable the hw timer, setting its tick period, and setup its interrupt
 */
static int sys_clock_driver_init(void)
{
	hwtimer_enable(NP_TICK_PERIOD_US);

	IRQ_CONNECT(TIMER_TICK_IRQ, 1, np_timer_isr, 0, 0);
	irq_enable(TIMER_TICK_IRQ);

	timer_core_init();

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
