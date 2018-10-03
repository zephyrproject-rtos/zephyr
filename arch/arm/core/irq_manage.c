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
#include <tracing.h>

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
 * priority levels which are reserved.
 *
 * @return N/A
 */
void _irq_priority_set(unsigned int irq, unsigned int prio, u32_t flags)
{
	/* The kernel may reserve some of the highest priority levels.
	 * So we offset the requested priority level with the number
	 * of priority levels reserved by the kernel.
	 */

#if CONFIG_ZERO_LATENCY_IRQS
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
 *
 * See https://github.com/zephyrproject-rtos/zephyr/issues/3056
 */

#ifdef CONFIG_SYS_POWER_MANAGEMENT
void _arch_isr_direct_pm(void)
{
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
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
		_sys_power_save_idle_exit(idle_val);
	}

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	irq_unlock(key);
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	__asm__ volatile("cpsie i" : : : "memory");
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

}
#endif

void _arch_isr_direct_header(void)
{
	z_sys_trace_isr_enter();
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

