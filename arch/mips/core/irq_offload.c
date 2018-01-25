/*
 * Copyright (c) 2017 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <irq.h>
#include <irq_offload.h>

volatile irq_offload_routine_t _offload_routine;
static volatile void *offload_param;

/*
 * Called by _enter_irq
 *
 * Just in case the offload routine itself generates an unhandled
 * exception, clear the offload_routine global before executing.
 */
void _irq_do_offload(void)
{
	irq_offload_routine_t tmp;

	if (!_offload_routine)
		return;

	tmp = _offload_routine;
	_offload_routine = NULL;

	tmp((void *)offload_param);
}

void irq_offload(irq_offload_routine_t routine, void *parameter)
{
	int key;

	key = irq_lock();
	_offload_routine = routine;
	offload_param = parameter;

	/* trigger sw interrupt */
	mips_biscr(CR_SINT0);

	irq_unlock(key);
}
