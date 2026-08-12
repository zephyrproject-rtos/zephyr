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
#include <zephyr/drivers/timer/system_timer_lpm.h>

/* Weak-linked noop defaults for optional driver interfaces*/

void __weak sys_clock_set_timeout(uint32_t ticks, bool idle)
{
}

void __weak sys_clock_idle_exit(void)
{
}

bool __weak z_sys_clock_lpm_companion_ready(void)
{
	return true;
}
