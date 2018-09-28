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

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_M_EXC_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_M_EXC_H_

#include <arch/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE

/* nothing */

#else

#include <arch/arm/cortex_m/cmsis.h>
#include <arch/arm/cortex_m/exc.h>
#include <irq_offload.h>

#ifdef CONFIG_IRQ_OFFLOAD
extern volatile irq_offload_routine_t offload_routine;
#endif

/* Writes to the AIRCR must be accompanied by a write of the value 0x05FA
 * to the Vector Key field, otherwise the writes are ignored.
 */
#define AIRCR_VECT_KEY_PERMIT_WRITE 0x05FAUL
/**
 *
 * @brief Find out if running in an ISR context
 *
 * The current executing vector is found in the IPSR register. We consider the
 * IRQs (exception 16 and up), and the PendSV and SYSTICK exceptions to be
 * interrupts. Taking a fault within an exception is also considered in
 * interrupt context.
 *
 * @return 1 if in ISR, 0 if not.
 */
static ALWAYS_INLINE int _IsInIsr(void)
{
	u32_t vector = __get_IPSR();

	/* IRQs + PendSV (14) + SYSTICK (15) are interrupts. */
	return (vector > 13)
#ifdef CONFIG_IRQ_OFFLOAD
		/* Only non-NULL if currently running an offloaded function */
		|| offload_routine != NULL
#endif
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
		/* On ARMv6-M there is no nested execution bit, so we check
		 * exception 3, hard fault, to a detect a nested exception.
		 */
		|| (vector == 3)
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
		/* If not in thread mode, and if RETTOBASE bit in ICSR is 0,
		 * then there are preempted active exceptions to execute.
		 */
#ifndef CONFIG_BOARD_QEMU_CORTEX_M3
		/* The polarity of RETTOBASE is incorrectly flipped in
		 * all but the very latest master tip of QEMU's NVIC driver,
		 * see commit "armv7m: Rewrite NVIC to not use any GIC code".
		 * Until QEMU 2.9 is released, and the SDK is updated to
		 * include it, skip this check in QEMU.
		 */
		|| (vector && !(SCB->ICSR & SCB_ICSR_RETTOBASE_Msk))
#endif /* CONFIG_BOARD_QEMU_CORTEX_M3 */
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
		;
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
static ALWAYS_INLINE void _ExcSetup(void)
{
	NVIC_SetPriority(PendSV_IRQn, 0xff);

#ifdef CONFIG_CPU_CORTEX_M_HAS_BASEPRI
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

#if defined(CONFIG_ARM_SECURE_FIRMWARE)
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
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
}

/**
 * @brief Clear Fault exceptions
 *
 * Clear out exceptions for Mem, Bus, Usage and Hard Faults
 *
 * @return N/A
 */
static ALWAYS_INLINE void _ClearFaults(void)
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

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_M_EXC_H_ */
