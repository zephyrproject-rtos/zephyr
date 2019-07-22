/*
 * Copyright (c) 2018 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Software interrupts utility code - SPARC implementation
 */

#include <kernel.h>
#include <irq_offload.h>
#include <arch/sparc/arch.h>

volatile irq_offload_routine_t offload_routine;
static void *offload_param;

/* Called by software trap */
void z_irq_do_offload(void)
{
	offload_routine(offload_param);
}

void irq_offload(irq_offload_routine_t routine, void *parameter)
{
	__ASSERT((z_sparc_get_psr() & PSR_ET) != 0,
		 "%s called with trap disabled\n", __func__);

	offload_routine = routine;
	offload_param = parameter;

	/* generate software trap */
	__asm__ volatile("ta %0" : : "i"(SOFTWARE_TRAP_IRQ_OFFLOAD));

	offload_routine = NULL;
}
