/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-M and Cortex-R power management
 *
 */

#include <toolchain.h>
#include <linker/sections.h>

_ASM_FILE_PROLOGUE

GTEXT(z_arm_cpu_idle_init)
GTEXT(z_arch_cpu_idle)
GTEXT(z_arch_cpu_atomic_idle)

#if defined(CONFIG_CPU_CORTEX_M)
#define _SCB_SCR 0xE000ED10

#define _SCB_SCR_SEVONPEND (1 << 4)
#define _SCB_SCR_SLEEPDEEP (1 << 2)
#define _SCB_SCR_SLEEPONEXIT (1 << 1)
#define _SCR_INIT_BITS _SCB_SCR_SEVONPEND
#endif

/**
 *
 * @brief Initialization of CPU idle
 *
 * Only called by z_arch_kernel_init(). Sets SEVONPEND bit once for the system's
 * duration.
 *
 * @return N/A
 *
 * C function prototype:
 *
 * void z_arm_cpu_idle_init(void);
 */

SECTION_FUNC(TEXT, z_arm_cpu_idle_init)
#if defined(CONFIG_CPU_CORTEX_M)
	ldr r1, =_SCB_SCR
	movs.n r2, #_SCR_INIT_BITS
	str r2, [r1]
#endif
	bx lr

SECTION_FUNC(TEXT, z_arch_cpu_idle)
#ifdef CONFIG_TRACING
	push {r0, lr}
	bl    sys_trace_idle
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	pop {r0, r1}
        mov lr, r1
#else
	pop {r0, lr}
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
#endif /* CONFIG_TRACING */

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) \
	|| defined(CONFIG_ARMV7_R)
	cpsie i
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	/* clear BASEPRI so wfi is awakened by incoming interrupts */
	eors.n r0, r0
	msr BASEPRI, r0
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

	wfi

	bx lr

SECTION_FUNC(TEXT, z_arch_cpu_atomic_idle)
#ifdef CONFIG_TRACING
	push {r0, lr}
	bl    sys_trace_idle
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
        pop {r0, r1}
        mov lr, r1
#else
	pop {r0, lr}
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
#endif /* CONFIG_TRACING */

	/*
	 * Lock PRIMASK while sleeping: wfe will still get interrupted by
	 * incoming interrupts but the CPU will not service them right away.
	 */
	cpsid i

	/*
	 * No need to set SEVONPEND, it's set once in z_CpuIdleInit() and never
	 * touched again.
	 */

	/* r0: interrupt mask from caller */

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) \
	|| defined(CONFIG_ARMV7_R)
	/* No BASEPRI, call wfe directly (SEVONPEND set in z_CpuIdleInit()) */
	wfe

	cmp r0, #0
	bne _irq_disabled
	cpsie i
_irq_disabled:

#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	/* r1: zero, for setting BASEPRI (needs a register) */
	eors.n r1, r1

	/* unlock BASEPRI so wfe gets interrupted by incoming interrupts */
	msr BASEPRI, r1

	wfe

	msr BASEPRI, r0
	cpsie i
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
	bx lr
