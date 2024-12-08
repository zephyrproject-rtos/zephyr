/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-A and Cortex-R power management
 *
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

#if defined(CONFIG_ARM_ON_EXIT_CPU_IDLE)
#include <soc_cpu_idle.h>
#endif

_ASM_FILE_PROLOGUE

GTEXT(arch_cpu_idle)
GTEXT(arch_cpu_atomic_idle)

.macro _sleep_if_allowed wait_instruction
#if defined(CONFIG_ARM_ON_ENTER_CPU_IDLE_HOOK)
	push	{r0, lr}
	bl	z_arm_on_enter_cpu_idle
	/* Skip the wait instruction if on_enter_cpu_idle() returns false. */
	cmp	r0, #0
	beq	_skip_\@
#endif /* CONFIG_ARM_ON_ENTER_CPU_IDLE_HOOK */

	/*
	 * Wait for all memory transactions to complete before entering low
	 * power state.
	 */
	dsb
	\wait_instruction

#if defined(CONFIG_ARM_ON_EXIT_CPU_IDLE)
	/* Inline the macro provided by SoC-specific code */
	SOC_ON_EXIT_CPU_IDLE
#endif /* CONFIG_ARM_ON_EXIT_CPU_IDLE */

#if defined(CONFIG_ARM_ON_ENTER_CPU_IDLE_HOOK)
_skip_\@:
	pop	{r0, lr}
#endif /* CONFIG_ARM_ON_ENTER_CPU_IDLE_HOOK */
.endm

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE
SECTION_FUNC(TEXT, arch_cpu_idle)
#ifdef CONFIG_TRACING
	push	{r0, lr}
	bl	sys_trace_idle
	pop	{r0, lr}
#endif /* CONFIG_TRACING */

	/* Enter low power state */
	_sleep_if_allowed wfi

	/*
	 * Clear PRIMASK and flush instruction buffer to immediately service
	 * the wake-up interrupt.
	 */
	cpsie	i
	isb

	bx	lr

#endif

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE
SECTION_FUNC(TEXT, arch_cpu_atomic_idle)
#ifdef CONFIG_TRACING
	push	{r0, lr}
	bl	sys_trace_idle
	pop	{r0, lr}
#endif /* CONFIG_TRACING */

	/*
	 * Lock PRIMASK while sleeping: wfe will still get interrupted by
	 * incoming interrupts but the CPU will not service them right away.
	 */
	cpsid	i

	/* r0: interrupt mask from caller */

	/* No BASEPRI, call wfe directly
	 */
	_sleep_if_allowed wfe

	cmp	r0, #0
	bne	_irq_disabled
	cpsie	i
_irq_disabled:

	bx	lr
#endif
