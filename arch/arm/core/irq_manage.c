/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-M and Cortex-R interrupt management
 *
 *
 * Interrupt management: enabling/disabling and dynamic ISR
 * connecting/replacing.  SW_ISR_TABLE_DYNAMIC has to be enabled for
 * connecting ISRs at runtime.
 */

#include <kernel.h>
#include <arch/cpu.h>
#if defined(CONFIG_CPU_CORTEX_M)
#include <arch/arm/cortex_m/cmsis.h>
#elif defined(CONFIG_CPU_CORTEX_R)
#include <device.h>
#include <irq_nextlevel.h>
#endif
#include <sys/__assert.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <sw_isr_table.h>
#include <irq.h>
#include <kernel_structs.h>
#include <debug/tracing.h>

extern void z_arm_reserved(void);

#if defined(CONFIG_CPU_CORTEX_M)
#define NUM_IRQS_PER_REG 32
#define REG_FROM_IRQ(irq) (irq / NUM_IRQS_PER_REG)
#define BIT_FROM_IRQ(irq) (irq % NUM_IRQS_PER_REG)

void z_arch_irq_enable(unsigned int irq)
{
	NVIC_EnableIRQ((IRQn_Type)irq);
}

void z_arch_irq_disable(unsigned int irq)
{
	NVIC_DisableIRQ((IRQn_Type)irq);
}

int z_arch_irq_is_enabled(unsigned int irq)
{
	return NVIC->ISER[REG_FROM_IRQ(irq)] & BIT(BIT_FROM_IRQ(irq));
}

/**
 * @internal
 *
 * @brief Set an interrupt's priority
 *
 * The priority is verified if ASSERT_ON is enabled. The maximum number
 * of priority levels is a little complex, as there are some hardware
 * priority levels which are reserved.
 *
 * @return N/A
 */
void z_arm_irq_priority_set(unsigned int irq, unsigned int prio, u32_t flags)
{
	/* The kernel may reserve some of the highest priority levels.
	 * So we offset the requested priority level with the number
	 * of priority levels reserved by the kernel.
	 */

#if defined(CONFIG_ZERO_LATENCY_IRQS)
	/* If we have zero latency interrupts, those interrupts will
	 * run at a priority level which is not masked by irq_lock().
	 * Our policy is to express priority levels with special properties
	 * via flags
	 */
	if (flags & IRQ_ZERO_LATENCY) {
		prio = _EXC_ZERO_LATENCY_IRQS_PRIO;
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
	__ASSERT(prio <= (BIT(DT_NUM_IRQ_PRIO_BITS) - 1),
		 "invalid priority %d! values must be less than %lu\n",
		 prio - _IRQ_PRIO_OFFSET,
		 BIT(DT_NUM_IRQ_PRIO_BITS) - (_IRQ_PRIO_OFFSET));
	NVIC_SetPriority((IRQn_Type)irq, prio);
}

#elif defined(CONFIG_CPU_CORTEX_R)
void z_arch_irq_enable(unsigned int irq)
{
	struct device *dev = _sw_isr_table[0].arg;

	irq_enable_next_level(dev, (irq >> 8) - 1);
}

void z_arch_irq_disable(unsigned int irq)
{
	struct device *dev = _sw_isr_table[0].arg;

	irq_disable_next_level(dev, (irq >> 8) - 1);
}

int z_arch_irq_is_enabled(unsigned int irq)
{
	struct device *dev = _sw_isr_table[0].arg;

	return irq_is_enabled_next_level(dev);
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
void z_arm_irq_priority_set(unsigned int irq, unsigned int prio, u32_t flags)
{
	struct device *dev = _sw_isr_table[0].arg;

	irq_set_priority_next_level(dev, (irq >> 8) - 1, prio, flags);
}

#endif

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

void z_arch_isr_direct_header(void)
{
	sys_trace_isr_enter();
}

#if defined(CONFIG_ARM_SECURE_FIRMWARE)
/**
 *
 * @brief Set the target security state for the given IRQ
 *
 * Function sets the security state (Secure or Non-Secure) targeted
 * by the given irq. It requires ARMv8-M MCU.
 * It is only compiled if ARM_SECURE_FIRMWARE is defined.
 * It should only be called while in Secure state, otherwise, a write attempt
 * to NVIC.ITNS register is write-ignored(WI), as the ITNS register is not
 * banked between security states and, therefore, has no Non-Secure instance.
 *
 * It shall assert if the operation is not performed successfully.
 *
 * @param irq IRQ line
 * @param secure_state 1 if target state is Secure, 0 otherwise.
 *
 * @return N/A
 */
void irq_target_state_set(unsigned int irq, int secure_state)
{
	if (secure_state) {
		/* Set target to Secure */
		if (NVIC_ClearTargetState(irq) != 0) {
			__ASSERT(0, "NVIC SetTargetState error");
		}
	} else {
		/* Set target state to Non-Secure */
		if (NVIC_SetTargetState(irq) != 1) {
			__ASSERT(0, "NVIC SetTargetState error");
		}
	}
}

/**
 *
 * @brief Determine whether the given IRQ targets the Secure state
 *
 * Function determines whether the given irq targets the Secure state
 * or not (i.e. targets the Non-Secure state). It requires ARMv8-M MCU.
 * It is only compiled if ARM_SECURE_FIRMWARE is defined.
 * It should only be called while in Secure state, otherwise, a read attempt
 * to NVIC.ITNS register is read-as-zero(RAZ), as the ITNS register is not
 * banked between security states and, therefore, has no Non-Secure instance.
 *
 * @param irq IRQ line
 *
 * @return 1 if target state is Secure, 0 otherwise.
 */
int irq_target_state_is_secure(unsigned int irq)
{
	return NVIC_GetTargetState(irq) == 0;
}

#endif /* CONFIG_ARM_SECURE_FIRMWARE */

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
