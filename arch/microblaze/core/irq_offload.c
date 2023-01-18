/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <zephyr/irq_offload.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>

#include <microblaze/emulate_isr.h>

#pragma message "MicroBlaze irq_offload is experimental"

volatile irq_offload_routine_t _offload_routine;
static volatile const void *offload_param;

/* Called by _enter_irq if regardless of pending irqs.
 * Just in case the offload routine itself reenables & generates
 * an interrupt, clear the offload_routine global before executing.
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
	microblaze_disable_interrupts();

	_offload_routine = routine;
	offload_param = parameter;

	EMULATE_ISR();
}
