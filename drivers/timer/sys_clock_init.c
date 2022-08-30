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

void __weak sys_clock_set_timeout(int32_t ticks, bool idle)
{
}

void __weak sys_clock_idle_exit(void)
{
}
