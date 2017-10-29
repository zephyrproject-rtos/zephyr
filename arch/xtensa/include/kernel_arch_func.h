/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* this file is only meant to be included by kernel_structs.h */

#ifndef _kernel_arch_func__h_
#define _kernel_arch_func__h_

#ifndef _ASMLANGUAGE

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
	_kernel.nested = 0;
#if XCHAL_CP_NUM > 0
	/* Initialize co-processor management for threads.
	 * Leave CPENABLE alone.
	 */
	_xt_coproc_init();
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
static ALWAYS_INLINE void
_set_thread_return_value(struct k_thread *thread, unsigned int value)
{
	thread->callee_saved.retval = value;
}

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

#define _is_in_isr() (_kernel.nested != 0)

#endif /* _ASMLANGUAGE */

#endif /* _kernel_arch_func__h_ */
