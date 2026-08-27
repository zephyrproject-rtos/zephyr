/*
 * Copyright (c) 2015 Intel corporation
 * Copyright 2025-2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Software interrupts utility code - ARM implementation
 */

#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>
#include <cmsis_core.h>

/* Called by z_arm_svc, which must preserve the callee-saved regs. r4 and r5. */
__attribute__((naked)) void z_irq_do_offload(void)
{
	__asm__ volatile("mov r0, r5; bx r4;");
	/* return from the offload routine branches back to the svc handler */
}

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) && !defined(CONFIG_ARMV8_M_BASELINE) \
	&& defined(CONFIG_ASSERT)
	/* ARMv6-M HardFault if you make a SVC call with interrupts locked.
	 */
	__ASSERT(__get_PRIMASK() == 0U, "irq_offload called with interrupts locked\n");
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE && CONFIG_ASSERT */

	register const void *r4 __asm__("r4") = routine;
	register const void *r5 __asm__("r5") = parameter;

	__asm__ volatile("svc %[id]\n" IF_ENABLED(CONFIG_ARM_BTI, ("bti"))
						  :
						  : [id] "i"(_SVC_CALL_IRQ_OFFLOAD), "r"(r4),
						    "r"(r5)
						  : "memory");
}

void arch_irq_offload_init(void)
{
}
