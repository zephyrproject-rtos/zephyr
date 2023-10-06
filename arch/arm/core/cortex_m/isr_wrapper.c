/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (c) 2023 Arm Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-M wrapper for ISRs with parameter
 *
 * Wrapper installed in vector table for handling dynamic interrupts that accept
 * a parameter.
 */
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/pm/pm.h>
#include <cmsis_core.h>

/**
 *
 * @brief Wrapper around ISRs when inserted in software ISR table
 *
 * When inserted in the vector table, _isr_wrapper() demuxes the ISR table
 * using the running interrupt number as the index, and invokes the registered
 * ISR with its corresponding argument. When returning from the ISR, it
 * determines if a context switch needs to happen (see documentation for
 * z_arm_pendsv()) and pends the PendSV exception if so: the latter will
 * perform the context switch itself.
 *
 */
void _isr_wrapper(void)
{
#ifdef CONFIG_TRACING_ISR
	sys_trace_isr_enter();
#endif /* CONFIG_TRACING_ISR */

#ifdef CONFIG_PM
	/*
	 * All interrupts are disabled when handling idle wakeup.  For tickless
	 * idle, this ensures that the calculation and programming of the
	 * device for the next timer deadline is not interrupted.  For
	 * non-tickless idle, this ensures that the clearing of the kernel idle
	 * state is not interrupted.  In each case, z_pm_save_idle_exit
	 * is called with interrupts disabled.
	 */

	/*
	 * Disable interrupts to prevent nesting while exiting idle state. This
	 * is only necessary for the Cortex-M because it is the only ARM
	 * architecture variant that automatically enables interrupts when
	 * entering an ISR.
	 */
	__disable_irq();

	/* is this a wakeup from idle ? */
	/* requested idle duration, in ticks */
	if (_kernel.idle != 0) {
		/* clear kernel idle state */
		_kernel.idle = 0;
		z_pm_save_idle_exit();
	}
	/* re-enable interrupts */
	__enable_irq();
#endif /* CONFIG_PM */

#if defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)
	int32_t irq_number = z_soc_irq_get_active();
#else
	/* _sw_isr_table does not map the expections, only the interrupts. */
	int32_t irq_number = __get_IPSR();
#endif
	irq_number -= 16;

	struct _isr_table_entry *entry = &_sw_isr_table[irq_number];
	(entry->isr)(entry->arg);

#if defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)
	z_soc_irq_eoi(irq_number);
#endif

#ifdef CONFIG_TRACING_ISR
	sys_trace_isr_exit();
#endif /* CONFIG_TRACING_ISR */

	z_arm_exc_exit();
}
