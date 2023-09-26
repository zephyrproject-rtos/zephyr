/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-M power management
 *
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

#if defined(CONFIG_ARM_ON_EXIT_CPU_IDLE)
#include <soc_cpu_idle.h>
#endif

_ASM_FILE_PROLOGUE

GTEXT(z_arm_cpu_idle_init)
GTEXT(arch_cpu_idle)
GTEXT(arch_cpu_atomic_idle)

#define _SCB_SCR		0xE000ED10

#define _SCB_SCR_SEVONPEND	(1 << 4)
#define _SCB_SCR_SLEEPDEEP	(1 << 2)
#define _SCB_SCR_SLEEPONEXIT	(1 << 1)
#define _SCR_INIT_BITS		_SCB_SCR_SEVONPEND

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
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	pop	{r0, r1}
	mov	lr, r1
#else
	pop	{r0, lr}
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
#endif /* CONFIG_ARM_ON_ENTER_CPU_IDLE_HOOK */
.endm

/**
 *
 * @brief Initialization of CPU idle
 *
 * Only called by arch_kernel_init(). Sets SEVONPEND bit once for the system's
 * duration.
 *
 * C function prototype:
 *
 * void z_arm_cpu_idle_init(void);
 */

SECTION_FUNC(TEXT, z_arm_cpu_idle_init)
	ldr	r1, =_SCB_SCR
	movs.n	r2, #_SCR_INIT_BITS
	str	r2, [r1]
	bx	lr

SECTION_FUNC(TEXT, arch_cpu_idle)
#if defined(CONFIG_TRACING) || \
    defined(CONFIG_ARM_ON_ENTER_CPU_IDLE_PREPARE_HOOK)
	push	{r0, lr}

#ifdef CONFIG_TRACING
	bl	sys_trace_idle
#endif
#ifdef CONFIG_ARM_ON_ENTER_CPU_IDLE_PREPARE_HOOK
	bl	z_arm_on_enter_cpu_idle_prepare
#endif

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	pop	{r0, r1}
	mov	lr, r1
#else
	pop	{r0, lr}
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
#endif

#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	/*
	 * PRIMASK is always cleared on ARMv7-M and ARMv8-M Mainline (not used
	 * for interrupt locking), and configuring BASEPRI to the lowest
	 * priority to ensure wake-up will cause interrupts to be serviced
	 * before entering low power state.
	 *
	 * Set PRIMASK before configuring BASEPRI to prevent interruption
	 * before wake-up.
	 */
	cpsid	i

	/*
	 * Set wake-up interrupt priority to the lowest and synchronise to
	 * ensure that this is visible to the WFI instruction.
	 */
	eors.n	r0, r0
	msr	BASEPRI, r0
	isb
#else
	/*
	 * For all the other ARM architectures that do not implement BASEPRI,
	 * PRIMASK is used as the interrupt locking mechanism, and it is not
	 * necessary to set PRIMASK here, as PRIMASK would have already been
	 * set by the caller as part of interrupt locking if necessary
	 * (i.e. if the caller sets _kernel.idle).
	 */
#endif /* CONFIG_ARMV7_M_ARMV8_M_MAINLINE */

	/* Enter low power state */
	_sleep_if_allowed wfi

	/*
	 * Clear PRIMASK and flush instruction buffer to immediately service
	 * the wake-up interrupt.
	 */
	cpsie	i
	isb

	bx	lr

SECTION_FUNC(TEXT, arch_cpu_atomic_idle)
#if defined(CONFIG_TRACING) || \
    defined(CONFIG_ARM_ON_ENTER_CPU_IDLE_PREPARE_HOOK)
	push	{r0, lr}

#ifdef CONFIG_TRACING
	bl	sys_trace_idle
#endif
#ifdef CONFIG_ARM_ON_ENTER_CPU_IDLE_PREPARE_HOOK
	bl	z_arm_on_enter_cpu_idle_prepare
#endif

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	pop	{r0, r1}
	mov	lr, r1
#else
	pop	{r0, lr}
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
#endif
	/*
	 * Lock PRIMASK while sleeping: wfe will still get interrupted by
	 * incoming interrupts but the CPU will not service them right away.
	 */
	cpsid	i

	/*
	 * No need to set SEVONPEND, it's set once in z_arm_cpu_idle_init()
	 * and never touched again.
	 */

	/* r0: interrupt mask from caller */

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	/* No BASEPRI, call wfe directly
	 * (SEVONPEND is set in z_arm_cpu_idle_init())
	 */
	_sleep_if_allowed wfe

	cmp	r0, #0
	bne	_irq_disabled
	cpsie	i
_irq_disabled:

#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	/* r1: zero, for setting BASEPRI (needs a register) */
	eors.n	r1, r1

	/* unlock BASEPRI so wfe gets interrupted by incoming interrupts */
	msr	BASEPRI, r1

	_sleep_if_allowed wfe

	msr	BASEPRI, r0
	cpsie	i
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
	bx	lr
