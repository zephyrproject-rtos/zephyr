/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-R System Control Block interface
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <misc/util.h>

/**
 *
 * @brief Reset the system
 *
 * This routine resets the processor.
 *
 * @return N/A
 */

void __weak sys_arch_reboot(int type)
{
	ARG_UNUSED(type);
}
