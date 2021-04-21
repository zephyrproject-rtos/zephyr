/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Exception/interrupt context helpers for Cortex-M CPUs
 *
 * Exception/interrupt context helpers.
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_M_EXC_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_M_EXC_H_

#include <arch/cpu.h>

#ifdef _ASMLANGUAGE

/* nothing */

#else

#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <arch/arm/aarch32/exc.h>
#include <irq_offload.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_IRQ_OFFLOAD
extern volatile irq_offload_routine_t offload_routine;
#endif

/* Writes to the AIRCR must be accompanied by a write of the value 0x05FA
 * to the Vector Key field, otherwise the writes are ignored.
 */
#define AIRCR_VECT_KEY_PERMIT_WRITE 0x05FAUL

/*
 * The current executing vector is found in the IPSR register. All
 * IRQs and system exceptions are considered as interrupt context.
 */
static ALWAYS_INLINE bool arch_is_in_isr(void)
{
	return (__get_IPSR()) ? (true) : (false);
}

/**
 * @brief Find out if we were in ISR context
 *        before the current exception occurred.
 *
 * A function that determines, based on inspecting the current
 * ESF, whether the processor was in handler mode before entering
 * the current exception state (i.e. nested exception) or not.
 *
 * Notes:
 * - The function shall only be called from ISR context.
 * - We do not use ARM processor state flags to determine
 *   whether we are in a nested exception; we rely on the
 *   RETPSR value stacked on the ESF. Hence, the function
 *   assumes that the ESF stack frame has a valid RETPSR
 *   value.
 *
 * @param esf the exception stack frame (cannot be NULL)
 * @return true if execution state was in handler mode, before
 *              the current exception occurred, otherwise false.
 */
static ALWAYS_INLINE bool arch_is_in_nested_exception(const z_arch_esf_t *esf)
{
	return (esf->basic.xpsr & IPSR_ISR_Msk) ? (true) : (false);
}

/**
 * @brief Setup system exceptions
 *
 * Set exception priorities to conform with the BASEPRI locking mechanism.
 * Set PendSV priority to lowest possible.
 *
 * Enable fault exceptions.
 *
 * @return N/A
 */
static ALWAYS_INLINE void z_arm_exc_setup(void)
{
	NVIC_SetPriority(PendSV_IRQn, _EXC_PENDSV_PRIO);

#ifdef CONFIG_CPU_CORTEX_M_HAS_BASEPRI
	/* Note: SVCall IRQ priority level is left to default (0)
	 * for Cortex-M variants without BASEPRI (e.g. ARMv6-M).
	 */
	NVIC_SetPriority(SVCall_IRQn, _EXC_SVC_PRIO);
#endif

#ifdef CONFIG_CPU_CORTEX_M_HAS_PROGRAMMABLE_FAULT_PRIOS
	NVIC_SetPriority(MemoryManagement_IRQn, _EXC_FAULT_PRIO);
	NVIC_SetPriority(BusFault_IRQn, _EXC_FAULT_PRIO);
	NVIC_SetPriority(UsageFault_IRQn, _EXC_FAULT_PRIO);
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
	NVIC_SetPriority(SecureFault_IRQn, _EXC_FAULT_PRIO);
#endif /* CONFIG_ARM_SECURE_FIRMWARE */

	/* Enable Usage, Mem, & Bus Faults */
	SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk |
		      SCB_SHCSR_BUSFAULTENA_Msk;
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
	/* Enable Secure Fault */
	SCB->SHCSR |= SCB_SHCSR_SECUREFAULTENA_Msk;
	/* Clear BFAR before setting BusFaults to target Non-Secure state. */
	SCB->BFAR = 0;
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
#endif /* CONFIG_CPU_CORTEX_M_HAS_PROGRAMMABLE_FAULT_PRIOS */

#if defined(CONFIG_ARM_SECURE_FIRMWARE) && \
	!defined(CONFIG_ARM_SECURE_BUSFAULT_HARDFAULT_NMI)
	/* Set NMI, Hard, and Bus Faults as Non-Secure.
	 * NMI and Bus Faults targeting the Secure state will
	 * escalate to a SecureFault or SecureHardFault.
	 */
	SCB->AIRCR =
		(SCB->AIRCR & (~(SCB_AIRCR_VECTKEY_Msk)))
		| SCB_AIRCR_BFHFNMINS_Msk
		| ((AIRCR_VECT_KEY_PERMIT_WRITE << SCB_AIRCR_VECTKEY_Pos) &
			SCB_AIRCR_VECTKEY_Msk);
	/* Note: Fault conditions that would generate a SecureFault
	 * in a PE with the Main Extension instead generate a
	 * SecureHardFault in a PE without the Main Extension.
	 */
#endif /* ARM_SECURE_FIRMWARE && !ARM_SECURE_BUSFAULT_HARDFAULT_NMI */
}

/**
 * @brief Clear Fault exceptions
 *
 * Clear out exceptions for Mem, Bus, Usage and Hard Faults
 *
 * @return N/A
 */
static ALWAYS_INLINE void z_arm_clear_faults(void)
{
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	/* Reset all faults */
	SCB->CFSR = SCB_CFSR_USGFAULTSR_Msk |
		    SCB_CFSR_MEMFAULTSR_Msk |
		    SCB_CFSR_BUSFAULTSR_Msk;

	/* Clear all Hard Faults - HFSR is write-one-to-clear */
	SCB->HFSR = 0xffffffff;
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
}

/**
 * @brief Assess whether a debug monitor event should be treated as an error
 *
 * This routine checks the status of a debug_monitor() exception, and
 * evaluates whether this needs to be considered as a processor error.
 *
 * @return true if the DM exception is a processor error, otherwise false
 */
bool z_arm_debug_monitor_event_error_check(void);

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_M_EXC_H_ */
