/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2023 Arm Limited
 * Copyright (c) 2026 Synaptics Incorporated
 * Author: Jisheng Zhang <jszhang@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ARM Cortex-A and Cortex-R power management
 */

#include <zephyr/kernel.h>
#include <zephyr/tracing/tracing.h>

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

	/* Enter low power state */
	SLEEP_IF_ALLOWED(__WFI);

	/*
	 * Clear PRIMASK and flush instruction buffer to immediately service
	 * the wake-up interrupt.
	 */
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

	/*
	 * Lock PRIMASK while sleeping: wfe will still get interrupted by
	 * incoming interrupts but the CPU will not service them right away.
	 */
	__disable_irq();

	/* No BASEPRI, call wfe directly
	 */
	SLEEP_IF_ALLOWED(__WFE);

	if (!key) {
		__enable_irq();
	}
}
#endif
