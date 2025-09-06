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

volatile irq_offload_routine_t offload_routine;
static const void *offload_param;

/* Called by z_arm_svc */
void z_irq_do_offload(void)
{
	offload_routine(offload_param);
}

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) && !defined(CONFIG_ARMV8_M_BASELINE) \
	&& defined(CONFIG_ASSERT)
	/* ARMv6-M HardFault if you make a SVC call with interrupts locked.
	 */
	__ASSERT(__get_PRIMASK() == 0U, "irq_offload called with interrupts locked\n");
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE && CONFIG_ASSERT */

	k_sched_lock();
	offload_routine = routine;
	offload_param = parameter;

	__asm__ volatile ("svc %[id]\n"
			  IF_ENABLED(CONFIG_ARM_BTI, ("bti"))
			  :
			  : [id] "i" (_SVC_CALL_IRQ_OFFLOAD)
			  : "memory");

	offload_routine = NULL;
	k_sched_unlock();
}

void arch_irq_offload_init(void)
{
}
