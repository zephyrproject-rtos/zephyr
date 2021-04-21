/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <irq.h>
#include <irq_offload.h>
#include <sys/printk.h>

volatile irq_offload_routine_t _offload_routine;
static volatile const void *offload_param;

/*
 * Called by _enter_irq
 *
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

	__asm__ volatile ("ecall");

	irq_unlock(key);
}
