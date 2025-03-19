/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-A and Cortex-R interrupt management
 *
 *
 * Interrupt management: enabling/disabling and dynamic ISR
 * connecting/replacing.  SW_ISR_TABLE_DYNAMIC has to be enabled for
 * connecting ISRs at runtime.
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/irq.h>
#include <zephyr/tracing/tracing.h>
#include <zephyr/pm/pm.h>

extern void z_arm_reserved(void);

/*
 * For Cortex-A and Cortex-R cores, the default interrupt controller is the ARM
 * Generic Interrupt Controller (GIC) and therefore the architecture interrupt
 * control functions are mapped to the GIC driver interface.
 *
 * When GIC is used together with other interrupt controller for
 * multi-level interrupts support (i.e. CONFIG_MULTI_LEVEL_INTERRUPTS
 * is enabled), the architecture interrupt control functions are mapped
 * to the SoC layer in `include/arch/arm/irq.h`.
 * The exported arm interrupt control functions which are wrappers of
 * GIC control could be used for SoC to do level 1 irq control to implement SoC
 * layer interrupt control functions.
 *
 * When a custom interrupt controller is used (i.e.
 * CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER is enabled), the architecture
 * interrupt control functions are mapped to the SoC layer in
 * `include/arch/arm/irq.h`.
 */

#if !defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)
void arm_irq_enable(unsigned int irq)
{
	arm_gic_irq_enable(irq);
}

void arm_irq_disable(unsigned int irq)
{
	arm_gic_irq_disable(irq);
}

int arm_irq_is_enabled(unsigned int irq)
{
	return arm_gic_irq_is_enabled(irq);
}

/**
 * @internal
 *
 * @brief Set an interrupt's priority
 *
 * The priority is verified if ASSERT_ON is enabled. The maximum number
 * of priority levels is a little complex, as there are some hardware
 * priority levels which are reserved: three for various types of exceptions,
 * and possibly one additional to support zero latency interrupts.
 */
void arm_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	arm_gic_irq_set_priority(irq, prio, flags);
}

#endif /* !CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER */

void z_arm_fatal_error(unsigned int reason, const struct arch_esf *esf);

/**
 *
 * @brief Spurious interrupt handler
 *
 * Installed in all _sw_isr_table slots at boot time. Throws an error if
 * called.
 *
 */
void z_irq_spurious(const void *unused)
{
	ARG_UNUSED(unused);

	z_arm_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}

#ifdef CONFIG_PM
void _arch_isr_direct_pm(void)
{
	unsigned int key;

	/* irq_lock() does what we want for this CPU */
	key = irq_lock();

	if (_kernel.idle) {
		_kernel.idle = 0;
		pm_system_resume();
	}

	irq_unlock(key);
}
#endif

#ifdef CONFIG_DYNAMIC_INTERRUPTS
#ifdef CONFIG_GEN_ISR_TABLES
int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*routine)(const void *parameter),
			     const void *parameter, uint32_t flags)
{
	z_isr_install(irq, routine, parameter);
	z_arm_irq_priority_set(irq, priority, flags);
	return irq;
}
#endif /* CONFIG_GEN_ISR_TABLES */

#ifdef CONFIG_DYNAMIC_DIRECT_INTERRUPTS
static inline void z_arm_irq_dynamic_direct_isr_dispatch(void)
{
	uint32_t irq = __get_IPSR() - 16;

	if (irq < IRQ_TABLE_SIZE) {
		struct _isr_table_entry *isr_entry = &_sw_isr_table[irq];

		isr_entry->isr(isr_entry->arg);
	}
}

ISR_DIRECT_DECLARE(z_arm_irq_direct_dynamic_dispatch_reschedule)
{
	z_arm_irq_dynamic_direct_isr_dispatch();

	return 1;
}

ISR_DIRECT_DECLARE(z_arm_irq_direct_dynamic_dispatch_no_reschedule)
{
	z_arm_irq_dynamic_direct_isr_dispatch();

	return 0;
}

#endif /* CONFIG_DYNAMIC_DIRECT_INTERRUPTS */

#endif /* CONFIG_DYNAMIC_INTERRUPTS */
