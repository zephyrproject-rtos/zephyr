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

void __weak sys_clock_unused(void)
{
	/* Forward the legacy contract: a driver that has not migrated to this
	 * hook stops its clock when it sees K_TICKS_FOREVER (UINT32_MAX once
	 * converted to the unsigned tick argument), the long-standing
	 * "no deadline" signal under CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE. A driver
	 * without that branch reads it as a maximal wait, which is just as
	 * good: timekeeping accuracy is already forfeit here, so it does not
	 * matter when or whether the next announce comes.
	 */
	sys_clock_set_timeout((uint32_t)K_TICKS_FOREVER, false);
}
