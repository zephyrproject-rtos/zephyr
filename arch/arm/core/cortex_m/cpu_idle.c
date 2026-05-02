/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2023 Arm Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-M power management
 */
#include <zephyr/kernel.h>
#include <cmsis_core.h>

#if defined(CONFIG_ARM_ON_EXIT_CPU_IDLE)
#include <soc_cpu_idle.h>
#endif

/**
 * @brief Initialization of CPU idle
 *
 * Only called by arch_kernel_init(). Sets SEVONPEND bit once for the system's
 * duration.
 */
void z_arm_cpu_idle_init(void)
{
	SCB->SCR = SCB_SCR_SEVONPEND_Msk;
}

#if defined(CONFIG_ARM_ON_EXIT_CPU_IDLE)
#define ON_EXIT_IDLE_HOOK SOC_ON_EXIT_CPU_IDLE
#else
#define ON_EXIT_IDLE_HOOK                                                                          \
	do {                                                                                       \
	} while (false)
#endif

#if defined(CONFIG_ARM_ON_ENTER_CPU_IDLE_HOOK)
#define SLEEP_IF_ALLOWED(wait_instr)                                                               \
	do {                                                                                       \
		/* Skip the wait instr if on_enter_cpu_idle returns false */                       \
		if (z_arm_on_enter_cpu_idle()) {                                                   \
			/* Wait for all memory transaction to complete */                          \
			/* before entering low power state. */                                     \
			__DSB();                                                                   \
			wait_instr();                                                              \
			/* Inline the macro provided by SoC-specific code */                       \
			ON_EXIT_IDLE_HOOK;                                                         \
		}                                                                                  \
	} while (false)
#else
#define SLEEP_IF_ALLOWED(wait_instr)                                                               \
	do {                                                                                       \
		__DSB();                                                                           \
		wait_instr();                                                                      \
		ON_EXIT_IDLE_HOOK;                                                                 \
	} while (false)
#endif

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE
void arch_cpu_idle(void)
{
#if defined(CONFIG_TRACING)
	sys_trace_idle();
#endif

#if CONFIG_ARM_ON_ENTER_CPU_IDLE_PREPARE_HOOK
	z_arm_on_enter_cpu_idle_prepare();
#endif

#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	/*
	 * PRIMASK is always cleared on ARMv7-M and ARMv8-M (not used
	 * for interrupt locking), and configuring BASEPRI to the lowest
	 * priority to ensure wake-up will cause interrupts to be serviced
	 * before entering low power state.
	 *
	 * Set PRIMASK before configuring BASEPRI to prevent interruption
	 * before wake-up.
	 */
	__disable_irq();

	/*
	 * Set wake-up interrupt priority to the lowest and synchronize to
	 * ensure that this is visible to the WFI instruction.
	 */
	__set_BASEPRI(0);
	__ISB();
#else
	/*
	 * For all the other ARM architectures that do not implement BASEPRI,
	 * PRIMASK is used as the interrupt locking mechanism, and it is not
	 * necessary to set PRIMASK here, as PRIMASK would have already been
	 * set by the caller as part of interrupt locking if necessary
	 * (i.e. if the caller sets _kernel.idle).
	 */
#endif

	SLEEP_IF_ALLOWED(__WFI);

#if defined(CONFIG_TRACING)
	sys_trace_idle_exit();
#endif
	__enable_irq();
	__ISB();
}
#endif

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE
void arch_cpu_atomic_idle(unsigned int key)
{
#if defined(CONFIG_TRACING)
	sys_trace_idle();
#endif

#if CONFIG_ARM_ON_ENTER_CPU_IDLE_PREPARE_HOOK
	z_arm_on_enter_cpu_idle_prepare();
#endif

	/*
	 * Lock PRIMASK while sleeping: wfe will still get interrupted by
	 * incoming interrupts but the CPU will not service them right away.
	 */
	__disable_irq();

	/*
	 * No need to set SEVONPEND, it's set once in z_arm_cpu_idle_init()
	 * and never touched again.
	 */

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	/* No BASEPRI, call wfe directly. (SEVONPEND is set in z_arm_cpu_idle_init()) */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	/* unlock BASEPRI so wfe gets interrupted by incoming interrupts  */
	__set_BASEPRI(0);
	__ISB();
#else
#error Unsupported architecture
#endif

	SLEEP_IF_ALLOWED(__WFE);

#if defined(CONFIG_TRACING)
	sys_trace_idle_exit();
#endif

	arch_irq_unlock(key);
#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	__enable_irq();
#endif
}
#endif
