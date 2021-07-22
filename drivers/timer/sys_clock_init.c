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

#include <kernel.h>
#include <init.h>
#include <drivers/timer/system_timer.h>

/* Weak-linked noop defaults for optional driver interfaces*/

void __weak sys_clock_isr(void *arg)
{
	__ASSERT_NO_MSG(false);
}

int __weak sys_clock_driver_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

int __weak sys_clock_device_ctrl(const struct device *dev,
				 uint32_t ctrl_command,
				 enum pm_device_state *state)
{
	return -ENOSYS;
}

void __weak sys_clock_set_timeout(int32_t ticks, bool idle)
{
}

void __weak sys_clock_idle_exit(void)
{
}

void __weak sys_clock_disable(void)
{
}

SYS_DEVICE_DEFINE("sys_clock", sys_clock_driver_init, sys_clock_device_ctrl,
		PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
