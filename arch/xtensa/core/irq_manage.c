/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stdio.h>
#include <zephyr/arch/xtensa/irq.h>
#include <zephyr/sys/__assert.h>

#include <kernel_arch_func.h>

#include <xtensa_internal.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/**
 * @internal
 *
 * @brief Set an interrupt's priority
 *
 * The priority is verified if ASSERT_ON is enabled.
 *
 * The priority is verified if ASSERT_ON is enabled. The maximum number of
 * priority levels is a little complex, as there are some hardware priority
 * levels which are reserved: three for various types of exceptions, and
 * possibly one additional to support zero latency interrupts.
 *
 * Valid values are from 1 to 6. Interrupts of priority 1 are not masked when
 * interrupts are locked system-wide, so care must be taken when using them.
 * ISR installed with priority 0 interrupts cannot make kernel calls.
 */
void z_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	__ASSERT(prio < XCHAL_EXCM_LEVEL + 1,
		 "invalid priority %d! values must be less than %d\n",
		 prio, XCHAL_EXCM_LEVEL + 1);
	/* TODO: Write code to set priority if this is ever possible on
	 * Xtensa
	 */
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
#ifndef CONFIG_MULTI_LEVEL_INTERRUPTS
int z_arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			       void (*routine)(const void *parameter),
			       const void *parameter, uint32_t flags)
{
	ARG_UNUSED(flags);
	ARG_UNUSED(priority);

	z_isr_install(irq, routine, parameter);
	return irq;
}
#else /* !CONFIG_MULTI_LEVEL_INTERRUPTS */
int z_arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			       void (*routine)(const void *parameter),
			       const void *parameter, uint32_t flags)
{
	return z_soc_irq_connect_dynamic(irq, priority, routine, parameter,
					 flags);
}
#endif /* !CONFIG_MULTI_LEVEL_INTERRUPTS */
#endif /* CONFIG_DYNAMIC_INTERRUPTS */

void z_irq_spurious(const void *arg)
{
	int irqs, ie;

	ARG_UNUSED(arg);

	__asm__ volatile("rsr.interrupt %0" : "=r"(irqs));
	__asm__ volatile("rsr.intenable %0" : "=r"(ie));
	LOG_ERR(" ** Spurious INTERRUPT(s) %p, INTENABLE = %p",
		(void *)irqs, (void *)ie);

#if XCHAL_NUM_INTERRUPTS > 32
	__asm__ volatile("rsr.interrupt1 %0" : "=r"(irqs));
	__asm__ volatile("rsr.intenable1 %0" : "=r"(ie));
	LOG_ERR(" ** Spurious INTERRUPT1(s) %p, INTENABLE1 = %p",
		(void *)irqs, (void *)ie);
#endif

#if XCHAL_NUM_INTERRUPTS > 64
	__asm__ volatile("rsr.interrupt2 %0" : "=r"(irqs));
	__asm__ volatile("rsr.intenable2 %0" : "=r"(ie));
	LOG_ERR(" ** Spurious INTERRUPT2(s) %p, INTENABLE2 = %p",
		(void *)irqs, (void *)ie);
#endif

#if XCHAL_NUM_INTERRUPTS > 96
	__asm__ volatile("rsr.interrupt3 %0" : "=r"(irqs));
	__asm__ volatile("rsr.intenable3 %0" : "=r"(ie));
	LOG_ERR(" ** Spurious INTERRUPT3(s) %p, INTENABLE3 = %p",
		(void *)irqs, (void *)ie);
#endif

	xtensa_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}

int xtensa_irq_is_enabled(unsigned int irq)
{
	uint32_t ie;

#if XCHAL_NUM_INTERRUPTS > 32
	switch (irq >> 5) {
	case 0:
		__asm__ volatile("rsr.intenable  %0" : "=r"(ie));
		break;
	case 1:
		__asm__ volatile("rsr.intenable1 %0" : "=r"(ie));
		break;
#if XCHAL_NUM_INTERRUPTS > 64
	case 2:
		__asm__ volatile("rsr.intenable2 %0" : "=r"(ie));
		break;
#endif
#if XCHAL_NUM_INTERRUPTS > 96
	case 3:
		__asm__ volatile("rsr.intenable3 %0" : "=r"(ie));
		break;
#endif
	default:
		ie = 0;
		break;
	}
#else
	__asm__ volatile("rsr.intenable %0" : "=r"(ie));
#endif

	return (ie & (1 << (irq & 31U))) != 0U;
}
