/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <kernel_internal.h>
#include <irq.h>
#include <irq_offload.h>

volatile irq_offload_routine_t _offload_routine;
static volatile const void *offload_param;

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

	/* Generate irq offload trap */
	mips_biscr(CR_SINT0);

	irq_unlock(key);
}
