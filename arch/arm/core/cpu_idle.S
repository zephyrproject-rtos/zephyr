/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM CORTEX-M3 power management
 *
 */

#include <offsets_short.h>
#include <toolchain.h>
#include <sections.h>
#include <arch/cpu.h>
#ifdef CONFIG_TICKLESS_IDLE
#include <kernel_structs.h>
#endif

_ASM_FILE_PROLOGUE

GTEXT(_CpuIdleInit)
#ifdef CONFIG_SYS_POWER_MANAGEMENT
GTEXT(_NanoIdleValGet)
GTEXT(_NanoIdleValClear)
#endif
GTEXT(k_cpu_idle)
GTEXT(k_cpu_atomic_idle)

#define _SCB_SCR 0xE000ED10

#define _SCB_SCR_SEVONPEND (1 << 4)
#define _SCB_SCR_SLEEPDEEP (1 << 2)
#define _SCB_SCR_SLEEPONEXIT (1 << 1)
#define _SCR_INIT_BITS _SCB_SCR_SEVONPEND

/**
 *
 * @brief Initialization of CPU idle
 *
 * Only called by nanoArchInit(). Sets SEVONPEND bit once for the system's
 * duration.
 *
 * @return N/A
 *
 * C function prototype:
 *
 * void _CpuIdleInit (void);
 */

SECTION_FUNC(TEXT, _CpuIdleInit)
	ldr r1, =_SCB_SCR
	movs.n r2, #_SCR_INIT_BITS
	str r2, [r1]
	bx lr

#ifdef CONFIG_SYS_POWER_MANAGEMENT

/**
 *
 * @brief Get the kernel idle setting
 *
 * Returns the kernel idle setting, in ticks. Only called by __systick().
 *
 * @return the requested number of ticks for the kernel to be idle
 *
 * C function prototype:
 *
 * int32_t _NanoIdleValGet (void);
 */

SECTION_FUNC(TEXT, _NanoIdleValGet)
	ldr r0, =_kernel
	ldr r0, [r0, #_kernel_offset_to_idle]
	bx lr

/**
 *
 * @brief Clear the kernel idle setting
 *
 * Sets the kernel idle setting to 0. Only called by __systick().
 *
 * @return N/A
 *
 * C function prototype:
 *
 * void _NanoIdleValClear (void);
 */

SECTION_FUNC(TEXT, _NanoIdleValClear)
	ldr r0, =_kernel
	eors.n r1, r1
	str r1, [r0, #_kernel_offset_to_idle]
	bx lr

#endif /* CONFIG_SYS_POWER_MANAGEMENT */

/**
 *
 * @brief Power save idle routine for ARM Cortex-M
 *
 * This function will be called by the ernel idle loop or possibly within
 * an implementation of _sys_power_save_idle in the kernel when the
 * '_sys_power_save_flag' variable is non-zero.  The ARM 'wfi' instruction
 * will be issued, causing a low-power consumption sleep mode.
 *
 * @return N/A
 *
 * C function prototype:
 *
 * void k_cpu_idle (void);
 */

SECTION_FUNC(TEXT, k_cpu_idle)
#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
	push {lr}
	bl    _sys_k_event_logger_enter_sleep
	pop {r0}
	mov lr, r0
#endif

#if defined(CONFIG_ARMV6_M)
	cpsie i
#elif defined(CONFIG_ARMV7_M)
	/* clear BASEPRI so wfi is awakened by incoming interrupts */
	eors.n r0, r0
	msr BASEPRI, r0
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */

	wfi

	bx lr

/**
 *
 * @brief Atomically re-enable interrupts and enter low power mode
 *
 * INTERNAL
 * The requirements for k_cpu_atomic_idle() are as follows:
 * 1) The enablement of interrupts and entering a low-power mode needs to be
 *    atomic, i.e. there should be no period of time where interrupts are
 *    enabled before the processor enters a low-power mode.  See the comments
 *    in k_lifo_get(), for example, of the race condition that occurs
 *    if this requirement is not met.
 *
 * 2) After waking up from the low-power mode, the interrupt lockout state
 *    must be restored as indicated in the 'imask' input parameter.
 *
 * @return N/A
 *
 * C function prototype:
 *
 * void k_cpu_atomic_idle (unsigned int imask);
 */

SECTION_FUNC(TEXT, k_cpu_atomic_idle)
#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
	push {lr}
	bl    _sys_k_event_logger_enter_sleep
	pop {r1}
	mov lr, r1
#endif

	/*
	 * Lock PRIMASK while sleeping: wfe will still get interrupted by
	 * incoming interrupts but the CPU will not service them right away.
	 */
	cpsid i

	/*
	 * No need to set SEVONPEND, it's set once in _CpuIdleInit() and never
	 * touched again.
	 */

	/* r0: interrupt mask from caller */

#if defined(CONFIG_ARMV6_M)
	/* No BASEPRI, call wfe directly (SEVONPEND set in _CpuIdleInit()) */
	wfe

	cmp r0, #0
	bne _irq_disabled
	cpsie i
_irq_disabled:

#elif defined(CONFIG_ARMV7_M)
	/* r1: zero, for setting BASEPRI (needs a register) */
	eors.n r1, r1

	/* unlock BASEPRI so wfe gets interrupted by incoming interrupts */
	msr BASEPRI, r1

	wfe

	msr BASEPRI, r0
	cpsie i
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M  */
	bx lr
