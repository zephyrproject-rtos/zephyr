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
#include <zephyr/sys_clock.h>
#include "nsi_hw_scheduler.h"
#include "nsi_timer_model.h"
#include "soc.h"

/* The cycle counter is nsi_hws_get_time(), which counts microseconds. */
#define TIMER_CORE_CYCLES_PER_SEC 1000000
#define TICK_PERIOD_US (TIMER_CORE_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/*
 * The native simulator has no compare register. Time is the microsecond
 * counter nsi_hws_get_time(), and the only lever is hwtimer_set_silent_ticks(),
 * which skips that many periodic tick interrupts before raising the next one.
 * It still fits the COMPARE model: timer_driver_cycle_get() reads the (64-bit,
 * non-wrapping) microsecond counter and timer_driver_set_compare() turns an absolute
 * deadline into the number of ticks to skip. The deadline is a full 64-bit
 * value (a 32-bit RELOAD reload would cap the fast-forward idle periods the
 * simulator relies on).
 */
#define TIMER_CORE_CYCLES_WIDTH 64
#define TIMER_CORE_BACKEND_COMPARE
#define TIMER_CORE_64BIT_CYCLES

static inline uint64_t timer_driver_cycle_get(void)
{
	return nsi_hws_get_time();
}

static void timer_driver_set_compare(uint64_t cycles)
{
	uint64_t now = nsi_hws_get_time();
	uint64_t rel = (cycles > now) ? (cycles - now) : 0U;

	/* The deadline is tick-aligned and the simulator only raises interrupts
	 * on tick boundaries, so round up to the boundary at or after it (now may
	 * be mid-tick, e.g. after a busy-wait). set_silent_ticks(n) skips n
	 * interrupts and raises the (n+1)th, so a deadline n boundaries out skips
	 * n - 1.
	 */
	int64_t ticks = (int64_t)((rel + TICK_PERIOD_US - 1U) / TICK_PERIOD_US);

	hwtimer_set_silent_ticks(ticks > 0 ? ticks - 1 : 0);
}

#include "system_timer_generic.h"

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
	hwtimer_enable(TICK_PERIOD_US);

	IRQ_CONNECT(TIMER_TICK_IRQ, 1, np_timer_isr, 0, 0);
	irq_enable(TIMER_TICK_IRQ);

	/* Seed the announce baseline from the microsecond counter and arm the
	 * first tick.
	 */
	timer_core_init();

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
