/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/irq_offload.h>

volatile irq_offload_routine_t _offload_routine;
static volatile const void *offload_param;

/* Called by _enter_irq if it was passed 0 for ipending.
 * Just in case the offload routine itself generates an unhandled
 * exception, clear the offload_routine global before executing.
 */
void z_irq_do_offload(void)
{
	irq_offload_routine_t tmp;

	if (!_offload_routine) {
		return;
	}

	tmp = _offload_routine;
	_offload_routine = NULL;

	tmp((const void *)offload_param);
}

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	unsigned int key;

	key = irq_lock();
	_offload_routine = routine;
	offload_param = parameter;

	__asm__ volatile ("trap");

	irq_unlock(key);
}

void arch_irq_offload_init(void)
{
}
