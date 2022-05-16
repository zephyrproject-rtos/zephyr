/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-A and Cortex-R System Control Block interface
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>

/**
 *
 * @brief Reset the system
 *
 * This routine resets the processor.
 *
 */

void __weak sys_arch_reboot(int type)
{
	ARG_UNUSED(type);
}
