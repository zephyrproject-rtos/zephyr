/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
extern void ReservedInterruptHandler(unsigned int);

/* Defined in xtensa_context.S */
extern void _xt_coproc_init(void);

/**
 *
 * @brief Performs architecture-specific initialization
 *
 * This routine performs architecture-specific initialization of the nanokernel.
 * Trivial stuff is done inline; more complex initialization is done via
 * function calls.
 *
 * @return N/A
 */
static ALWAYS_INLINE void nanoArchInit(void)
{
	_kernel.nested = 0;
#if XCHAL_CP_NUM > 0
	/* Initialize co-processor management for tasks. Leave CPENABLE alone. */
	_xt_coproc_init();
#endif
}

/**
 *
 * @brief Set the return value for the specified fiber (inline)
 *
 * @param fiber pointer to fiber
 * @param value value to set as return value
 *
 * The register used to store the return value from a function call invocation
 * is set to <value>.  It is assumed that the specified <fiber> is pending, and
 * thus the fibers context is stored in its TCS.
 *
 * @return N/A
 */
static ALWAYS_INLINE void
_set_thread_return_value(struct k_thread *thread, unsigned int value)
{
	/* write into 'eax' slot created in _Swap() entry */

	*(unsigned int *)(thread->callee_saved.retval) = value;
}

extern void nano_cpu_atomic_idle(unsigned int imask);

/*
 * _IntLibInit() is called from the non-arch specific nanokernel function,
 * _nano_init(). The IA-32 nanokernel does not require any special
 * initialization of the interrupt subsystem. However, we still need to
 * provide an _IntLibInit() of some sort to prevent build errors.
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
