/*
 * Copyright (c) 2015 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Software interrupts utility code - ARM implementation
 */

#include <kernel.h>
#include <irq_offload.h>

volatile irq_offload_routine_t offload_routine;
static void *offload_param;

/* Called by __svc */
void _irq_do_offload(void)
{
	offload_routine(offload_param);
}

void irq_offload(irq_offload_routine_t routine, void *parameter)
{
#if defined(CONFIG_ARMV6_M) && defined(CONFIG_ASSERT)
	/* Cortex M0 hardfaults if you make a SVC call with interrupts
	 * locked.
	 */
	unsigned int key;

	__asm__ volatile("mrs %0, PRIMASK;" : "=r" (key) : : "memory");
	__ASSERT(key == 0, "irq_offload called with interrupts locked\n");
#endif

	k_sched_lock();
	offload_routine = routine;
	offload_param = parameter;

	__asm__ volatile ("svc %[id]"
			  :
			  : [id] "i" (_SVC_CALL_IRQ_OFFLOAD)
			  : "memory");

	offload_routine = NULL;
	k_sched_unlock();
}
