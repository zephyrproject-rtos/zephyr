/*
 * SPDX-FileCopyrightText: Copyright Alif Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Software interrupts utility code - ARM Cortex-A/R implementation
 *
 * On Cortex-A/R, irq_offload() is triggered via an svc that is dispatched by
 * z_arm_svc() (see switch.S), which calls the routine in interrupt context
 * on the initiating CPU.
 */

#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	/* z_arm_svc reads the routine and its parameter from the saved r0 and r1 */
	register irq_offload_routine_t r0 __asm__("r0") = routine;
	register const void *r1 __asm__("r1") = parameter;

	__asm__ volatile ("svc %[id]"
			  :
			  : [id] "i" (_SVC_CALL_IRQ_OFFLOAD), "r" (r0), "r" (r1)
			  : "memory");
}

void arch_irq_offload_init(void)
{
}
