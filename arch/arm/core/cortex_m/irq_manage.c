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

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <cmsis_core.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/irq.h>
#include <zephyr/tracing/tracing.h>
#include <zephyr/pm/pm.h>

extern void z_arm_reserved(void);

#define NUM_IRQS_PER_REG 32
#define REG_FROM_IRQ(irq) (irq / NUM_IRQS_PER_REG)
#define BIT_FROM_IRQ(irq) (irq % NUM_IRQS_PER_REG)

#if !defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)

void arch_irq_enable(unsigned int irq)
{
	NVIC_EnableIRQ((IRQn_Type)irq);
}

void arch_irq_disable(unsigned int irq)
{
	NVIC_DisableIRQ((IRQn_Type)irq);
}

int arch_irq_is_enabled(unsigned int irq)
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
 */
void z_arm_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	/* The kernel may reserve some of the highest priority levels.
	 * So we offset the requested priority level with the number
	 * of priority levels reserved by the kernel.
	 */

	/* If we have zero latency interrupts, those interrupts will
	 * run at a priority level which is not masked by irq_lock().
	 * Our policy is to express priority levels with special properties
	 * via flags
	 */
	if (IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS) && (flags & IRQ_ZERO_LATENCY)) {
		if (ZERO_LATENCY_LEVELS == 1) {
			prio = _EXC_ZERO_LATENCY_IRQS_PRIO;
		} else {
			/* Use caller supplied prio level as-is */
		}
	} else {
		prio += _IRQ_PRIO_OFFSET;
	}

	/* The last priority level is also used by PendSV exception, but
	 * allow other interrupts to use the same level, even if it ends up
	 * affecting performance (can still be useful on systems with a
	 * reduced set of priorities, like Cortex-M0/M0+).
	 */
	__ASSERT(prio <= (BIT(NUM_IRQ_PRIO_BITS) - 1),
		 "invalid priority %d for %d irq! values must be less than %lu\n",
		 prio - _IRQ_PRIO_OFFSET, irq,
		 BIT(NUM_IRQ_PRIO_BITS) - (_IRQ_PRIO_OFFSET));
	NVIC_SetPriority((IRQn_Type)irq, prio);
}

#endif /* !defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER) */

void z_arm_fatal_error(unsigned int reason, const z_arch_esf_t *esf);

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
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	unsigned int key;

	/* irq_lock() does what we want for this CPU */
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
		_kernel.idle = 0;
		z_pm_save_idle_exit();
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
 * It shall return the resulting target state of the given IRQ, indicating
 * whether the operation has been performed successfully.
 *
 * @param irq IRQ line
 * @param irq_target_state the desired IRQ target state
 *
 * @return The resulting target state of the given IRQ
 */
irq_target_state_t irq_target_state_set(unsigned int irq,
	irq_target_state_t irq_target_state)
{
	uint32_t result;

	if (irq_target_state == IRQ_TARGET_STATE_SECURE) {
		/* Set target to Secure */
		result = NVIC_ClearTargetState(irq);
	} else {
		/* Set target to Non-Secure */
		result = NVIC_SetTargetState(irq);
	}

	if (result) {
		return IRQ_TARGET_STATE_NON_SECURE;
	} else {
		return IRQ_TARGET_STATE_SECURE;
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

/**
 *
 * @brief Disable and set all interrupt lines to target Non-Secure state.
 *
 * The function is used to set all HW NVIC interrupt lines to target the
 * Non-Secure state. The function shall only be called fron Secure state.
 *
 * Notes:
 * - All NVIC interrupts are disabled before being routed to Non-Secure.
 * - Bits corresponding to un-implemented interrupts are RES0, so writes
 *   will be ignored.
 *
*/
void irq_target_state_set_all_non_secure(void)
{
	int i;

	/* Disable (Clear) all NVIC interrupt lines. */
	for (i = 0; i < sizeof(NVIC->ICER) / sizeof(NVIC->ICER[0]); i++) {
		NVIC->ICER[i] = 0xFFFFFFFF;
	}

	barrier_dsync_fence_full();
	barrier_isync_fence_full();

	/* Set all NVIC interrupt lines to target Non-Secure */
	for (i = 0; i < sizeof(NVIC->ITNS) / sizeof(NVIC->ITNS[0]); i++) {
		NVIC->ITNS[i] = 0xFFFFFFFF;
	}
}

#endif /* CONFIG_ARM_SECURE_FIRMWARE */

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
