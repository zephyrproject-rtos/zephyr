/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-M interrupt management
 *
 *
 * Interrupt management: enabling/disabling and dynamic ISR
 * connecting/replacing.  SW_ISR_TABLE_DYNAMIC has to be enabled for
 * connecting ISRs at runtime.
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <arch/arm/cortex_m/cmsis.h>
#include <misc/__assert.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <sw_isr_table.h>
#include <irq.h>
#include <kernel_structs.h>
#include <logging/kernel_event_logger.h>

extern void __reserved(void);

#define NUM_IRQS_PER_REG 32
#define REG_FROM_IRQ(irq) (irq / NUM_IRQS_PER_REG)
#define BIT_FROM_IRQ(irq) (irq % NUM_IRQS_PER_REG)

/**
 *
 * @brief Enable an interrupt line
 *
 * Enable the interrupt. After this call, the CPU will receive interrupts for
 * the specified <irq>.
 *
 * @return N/A
 */
void _arch_irq_enable(unsigned int irq)
{
	NVIC_EnableIRQ((IRQn_Type)irq);
}

/**
 *
 * @brief Disable an interrupt line
 *
 * Disable an interrupt line. After this call, the CPU will stop receiving
 * interrupts for the specified <irq>.
 *
 * @return N/A
 */
void _arch_irq_disable(unsigned int irq)
{
	NVIC_DisableIRQ((IRQn_Type)irq);
}

/**
 * @brief Return IRQ enable state
 *
 * @param irq IRQ line
 * @return interrupt enable state, true or false
 */
int _arch_irq_is_enabled(unsigned int irq)
{
	return NVIC->ISER[REG_FROM_IRQ(irq)] & (1 << BIT_FROM_IRQ(irq));
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
 *
 * @return N/A
 */
void _irq_priority_set(unsigned int irq, unsigned int prio, u32_t flags)
{
	/* Hardware priority levels 0 and 1 reserved for Kernel use.
	 * So we add 2 to the requested priority level. If we support
	 * ZLI, 2 is also reserved so we add 3.
	 */

#if CONFIG_ZERO_LATENCY_IRQS
	/* If we have zero latency interrupts, that makes priority level 2
	 * a case with special semantics; it is not masked by irq_lock().
	 * Our policy is to express priority levels with special properties
	 * via flags
	 */
	if (flags & IRQ_ZERO_LATENCY) {
		prio = 2;
	} else {
		prio += _IRQ_PRIO_OFFSET;
	}
#else
	ARG_UNUSED(flags);
	prio += _IRQ_PRIO_OFFSET;
#endif
	/* The last priority level is also used by PendSV exception, but
	 * allow other interrupts to use the same level, even if it ends up
	 * affecting performance (can still be useful on systems with a
	 * reduced set of priorities, like Cortex-M0/M0+).
	 */
	__ASSERT(prio <= ((1 << CONFIG_NUM_IRQ_PRIO_BITS) - 1),
		 "invalid priority %d! values must be less than %d\n",
		 prio - _IRQ_PRIO_OFFSET,
		 (1 << CONFIG_NUM_IRQ_PRIO_BITS) - (_IRQ_PRIO_OFFSET));
	NVIC_SetPriority((IRQn_Type)irq, prio);
}

/**
 *
 * @brief Spurious interrupt handler
 *
 * Installed in all dynamic interrupt slots at boot time. Throws an error if
 * called.
 *
 * See __reserved().
 *
 * @return N/A
 */
void _irq_spurious(void *unused)
{
	ARG_UNUSED(unused);
	__reserved();
}

/* FIXME: IRQ direct inline functions have to be placed here and not in
 * arch/cpu.h as inline functions due to nasty circular dependency between
 * arch/cpu.h and kernel_structs.h; the inline functions typically need to
 * perform operations on _kernel.  For now, leave as regular functions, a
 * future iteration will resolve this.
 * We have a similar issue with the k_event_logger functions.
 *
 * See https://jira.zephyrproject.org/browse/ZEP-1595
 */

#ifdef CONFIG_SYS_POWER_MANAGEMENT
void _arch_isr_direct_pm(void)
{
#if defined(CONFIG_ARMV6_M)
	int key;

	/* irq_lock() does what we wan for this CPU */
	key = irq_lock();
#elif defined(CONFIG_ARMV7_M)
	/* Lock all interrupts. irq_lock() will on this CPU only disable those
	 * lower than BASEPRI, which is not what we want. See comments in
	 * arch/arm/core/isr_wrapper.S
	 */
	__asm__ volatile("cpsid i" : : : "memory");
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */

	if (_kernel.idle) {
		s32_t idle_val = _kernel.idle;

		_kernel.idle = 0;
		_sys_power_save_idle_exit(idle_val);
	}

#if defined(CONFIG_ARMV6_M)
	irq_unlock(key);
#elif defined(CONFIG_ARMV7_M)
	__asm__ volatile("cpsie i" : : : "memory");
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */

}
#endif

#if defined(CONFIG_KERNEL_EVENT_LOGGER_SLEEP) || \
	defined(CONFIG_KERNEL_EVENT_LOGGER_INTERRUPT)
void _arch_isr_direct_header(void)
{
	_sys_k_event_logger_interrupt();
	_sys_k_event_logger_exit_sleep();
}
#endif

