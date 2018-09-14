/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* this file is only meant to be included by kernel_structs.h */

#ifndef ZEPHYR_ARCH_XTENSA_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_XTENSA_INCLUDE_KERNEL_ARCH_FUNC_H_

#ifndef _ASMLANGUAGE
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* stack alignment related macros: STACK_ALIGN_SIZE is defined above */

#define STACK_ROUND_UP(x) ROUND_UP(x, STACK_ALIGN_SIZE)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, STACK_ALIGN_SIZE)

extern void FatalErrorHandler(void);
extern void ReservedInterruptHandler(unsigned int intNo);

/* Defined in xtensa_context.S */
extern void _xt_coproc_init(void);

extern K_THREAD_STACK_DEFINE(_interrupt_stack, CONFIG_ISR_STACK_SIZE);

static ALWAYS_INLINE _cpu_t *_arch_curr_cpu(void)
{
#ifdef CONFIG_XTENSA_ASM2
	void *val;

	__asm__ volatile("rsr.misc0 %0" : "=r"(val));

	return val;
#else
	return &_kernel.cpus[0];
#endif
}

/**
 *
 * @brief Performs architecture-specific initialization
 *
 * This routine performs architecture-specific initialization of the
 * kernel.  Trivial stuff is done inline; more complex initialization is
 * done via function calls.
 *
 * @return N/A
 */
static ALWAYS_INLINE void kernel_arch_init(void)
{
	_cpu_t *cpu0 = &_kernel.cpus[0];

	cpu0->nested = 0;

#if CONFIG_XTENSA_ASM2
	cpu0->irq_stack = (K_THREAD_STACK_BUFFER(_interrupt_stack) +
			   CONFIG_ISR_STACK_SIZE);

	/* The asm2 scheme keeps the kernel pointer in MISC0 for easy
	 * access.  That saves 4 bytes of immediate value to store the
	 * address when compared to the legacy scheme.  But in SMP
	 * this record is a per-CPU thing and having it stored in a SR
	 * already is a big win.
	 */
	__asm__ volatile("wsr.MISC0 %0; rsync" : : "r"(cpu0));

#endif

#if !defined(CONFIG_XTENSA_ASM2) && XCHAL_CP_NUM > 0
	/* Initialize co-processor management for threads.
	 * Leave CPENABLE alone.
	 */
	_xt_coproc_init();
#endif

#ifdef CONFIG_INIT_STACKS
	memset(K_THREAD_STACK_BUFFER(_interrupt_stack), 0xAA,
	       CONFIG_ISR_STACK_SIZE);
#endif
}

/**
 *
 * @brief Set the return value for the specified thread (inline)
 *
 * @param thread pointer to thread
 * @param value value to set as return value
 *
 * The register used to store the return value from a function call invocation
 * is set to <value>.  It is assumed that the specified thread is pending, and
 * thus the thread's context is stored in its k_thread.
 *
 * @return N/A
 */
#if !CONFIG_USE_SWITCH
static ALWAYS_INLINE void
_set_thread_return_value(struct k_thread *thread, unsigned int value)
{
	thread->callee_saved.retval = value;
}
#endif

extern void k_cpu_atomic_idle(unsigned int imask);

/*
 * Required by the core kernel even though we don't have to do anything on this
 * arch.
 */
static inline void _IntLibInit(void)
{
}

#include <stddef.h> /* For size_t */

#ifdef __cplusplus
}
#endif

#define _is_in_isr() (_arch_curr_cpu()->nested != 0)

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_XTENSA_INCLUDE_KERNEL_ARCH_FUNC_H_ */
