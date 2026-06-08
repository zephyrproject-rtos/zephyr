/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2023 Arm Limited
 * Copyright (c) 2026 Synaptics Incorporated
 * Author: Jisheng Zhang <jszhang@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ARM1176JZF-S power management.
 *
 * Uses the ARMv6 CP15 wait-for-interrupt operation instead of the
 * ARMv7-only WFI mnemonic.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/tracing/tracing.h>

static ALWAYS_INLINE void arm1176_wait_for_interrupt(void)
{
	uint32_t zero = 0U;

	/* CP15 WFI: wait for interrupt */
	__asm__ volatile("mcr p15, 0, %0, c7, c0, 4" : : "r"(zero) : "memory");
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
		if (z_arm_on_enter_cpu_idle()) {                                                   \
			barrier_dsync_fence_full();                                                \
			wait_instr();                                                              \
			ON_EXIT_IDLE_HOOK;                                                         \
		}                                                                                  \
	} while (false)
#else
#define SLEEP_IF_ALLOWED(wait_instr)                                                               \
	do {                                                                                       \
		barrier_dsync_fence_full();                                                        \
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

	SLEEP_IF_ALLOWED(arm1176_wait_for_interrupt);

	__enable_irq();
	barrier_isync_fence_full();
}
#endif

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE
void arch_cpu_atomic_idle(unsigned int key)
{
#if defined(CONFIG_TRACING)
	sys_trace_idle();
#endif

	__disable_irq();

	SLEEP_IF_ALLOWED(arm1176_wait_for_interrupt);

	if (!key) {
		__enable_irq();
	}
}
#endif
