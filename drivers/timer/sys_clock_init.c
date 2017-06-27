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
#include <drivers/system_timer.h>

SYS_DEVICE_DEFINE("sys_clock", _sys_clock_driver_init, sys_clock_device_ctrl,
		PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
