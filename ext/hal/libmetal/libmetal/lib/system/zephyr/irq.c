/*
 * Copyright (c) 2017, Linaro Limited. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	zephyr/irq.c
 * @brief	Zephyr libmetal irq definitions.
 */

#include <errno.h>
#include <metal/irq.h>
#include <metal/sys.h>
#include <metal/log.h>
#include <metal/mutex.h>
#include <metal/list.h>
#include <metal/utilities.h>
#include <metal/alloc.h>

unsigned int metal_irq_save_disable(void)
{
	return irq_lock();
}

void metal_irq_restore_enable(unsigned int flags)
{
	irq_unlock(flags);
}

