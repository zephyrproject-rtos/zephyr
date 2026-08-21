/*
 * Copyright (c) 2015 Intel corporation
 * Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Software interrupts utility code - ARM implementation
 */

#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>
#include <cmsis_core.h>

static volatile irq_offload_routine_t offload_routine[CONFIG_MP_MAX_NUM_CPUS];
static const void *offload_param[CONFIG_MP_MAX_NUM_CPUS];

/* Called by z_arm_svc */
void z_irq_do_offload(void)
{
	unsigned int id = arch_curr_cpu()->id;

	offload_routine[id](offload_param[id]);
}

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) && !defined(CONFIG_ARMV8_M_BASELINE) \
	&& defined(CONFIG_ASSERT)
	/* ARMv6-M HardFault if you make a SVC call with interrupts locked.
	 */
	__ASSERT(__get_PRIMASK() == 0U, "irq_offload called with interrupts locked\n");
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE && CONFIG_ASSERT */

	unsigned int id = arch_curr_cpu()->id;

	k_sched_lock();
	offload_routine[id] = routine;
	offload_param[id] = parameter;

	__asm__ volatile ("svc %[id]\n"
			  IF_ENABLED(CONFIG_ARM_BTI, ("bti"))
			  :
			  : [id] "i" (_SVC_CALL_IRQ_OFFLOAD)
			  : "memory");

	offload_routine[id] = NULL;
	k_sched_unlock();
}

void arch_irq_offload_init(void)
{
}
