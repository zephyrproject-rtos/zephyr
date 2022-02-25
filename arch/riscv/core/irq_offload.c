/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <irq.h>
#include <irq_offload.h>
#include <arch/riscv/syscall.h>

static irq_offload_routine_t offload_routine;
static const void *offload_param;

/*
 * Called by _enter_irq
 */
void z_irq_do_offload(void)
{
	offload_routine(offload_param);
}

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	unsigned int key;

	key = irq_lock();
	offload_routine = routine;
	offload_param = parameter;
	arch_syscall_invoke0(RV_ECALL_IRQ_OFFLOAD);
	irq_unlock(key);
}
