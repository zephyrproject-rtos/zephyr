/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM interrupt handler
 *
 * Interrupt management: dynamic ISR connecting/replacing;
 * SW_ISR_TABLE_DYNAMIC has to be enabled for connecting ISRs at runtime.
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <sys/__assert.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <sw_isr_table.h>
#include <irq.h>
#include <kernel_structs.h>
#include <debug/tracing.h>

extern void z_arm_reserved(void);

/**
 *
 * @brief Spurious interrupt handler
 *
 * Installed in all dynamic interrupt slots at boot time. Throws an error if
 * called.
 *
 * See z_arm_reserved().
 *
 * @return N/A
 */
void z_irq_spurious(void *unused)
{
	ARG_UNUSED(unused);
	z_arm_reserved();
}

/* FIXME: IRQ direct inline functions have to be placed here and not in
 * arch/cpu.h as inline functions due to nasty circular dependency between
 * arch/cpu.h and kernel_structs.h; the inline functions typically need to
 * perform operations on _kernel.  For now, leave as regular functions, a
 * future iteration will resolve this.
 *
 * See https://github.com/zephyrproject-rtos/zephyr/issues/3056
 */

#ifdef CONFIG_SYS_POWER_MANAGEMENT
void _arch_isr_direct_pm(void)
{
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) \
	|| defined(CONFIG_ARMV7_R)
	unsigned int key;

	/* irq_lock() does what we wan for this CPU */
	key = irq_lock();
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	/* Lock all interrupts. irq_lock() will on this CPU only disable those
	 * lower than BASEPRI, which is not what we want. See comments in
	 * arch/arm/core/isr_wrapper.S
	 */
	__asm__ volatile("cpsid i" : : : "memory");
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

	if (_kernel.idle) {
		s32_t idle_val = _kernel.idle;

		_kernel.idle = 0;
		z_sys_power_save_idle_exit(idle_val);
	}

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) \
	|| defined(CONFIG_ARMV7_R)
	irq_unlock(key);
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	__asm__ volatile("cpsie i" : : : "memory");
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

}
#endif

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int z_arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			      void (*routine)(void *parameter), void *parameter,
			      u32_t flags)
{
	z_isr_install(irq, routine, parameter);
	z_arm_irq_priority_set(irq, priority, flags);
	return irq;
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS */
