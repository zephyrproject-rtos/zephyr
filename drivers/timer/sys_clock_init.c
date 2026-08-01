/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Initialize system clock driver
 *
 * Initializing the timer driver is done in this module to reduce code
 * duplication.
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>

/* Weak-linked noop defaults for optional driver interfaces*/

void __weak sys_clock_set_timeout(uint32_t ticks, bool idle)
{
}

void __weak sys_clock_idle_exit(void)
{
}

void __weak sys_clock_no_timeout(void)
{
	/* A driver that does not implement this hook may recognise
	 * K_TICKS_FOREVER as the request to stop its clock and act on it;
	 * one that does not reads it as a maximal wait, which is equally
	 * acceptable here since timekeeping accuracy is already forfeit.
	 * That tick value is deprecated in favour of implementing this hook.
	 */
	sys_clock_set_timeout((uint32_t)K_TICKS_FOREVER, false);
}

void __weak sys_clock_idle_enter(uint32_t ticks)
{
	/* A driver that does not implement this hook may still key its
	 * low-power handling on sys_clock_set_timeout()'s idle argument, so
	 * pass true here. That argument is deprecated in favour of
	 * implementing this hook, and carries no information for a driver
	 * that does.
	 */
	sys_clock_set_timeout(ticks, true);
}
