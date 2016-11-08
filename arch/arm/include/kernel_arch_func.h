/*
 * Copyright (c) 2013-2016 Wind River Systems, Inc.
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

/**
 * @file
 * @brief Private kernel definitions (ARM)
 *
 * This file contains private kernel function definitions and various
 * other definitions for the ARM Cortex-M3 processor architecture.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that kernel
 * assembly source files obtains structure offset values via "absolute symbols"
 * in the offsets.o module.
 */

/* this file is only meant to be included by kernel_structs.h */

#ifndef _kernel_arch_func__h_
#define _kernel_arch_func__h_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
extern void _FaultInit(void);
extern void _CpuIdleInit(void);
static ALWAYS_INLINE void nanoArchInit(void)
{
	_InterruptStackSetup();
	_ExcSetup();
	_FaultInit();
	_CpuIdleInit();
}

/**
 *
 * @brief Set the return value for the specified fiber (inline)
 *
 * The register used to store the return value from a function call invocation
 * to <value>.  It is assumed that the specified <fiber> is pending, and thus
 * the fiber's thread is stored in its struct tcs structure.
 *
 * @param fiber pointer to the fiber
 * @param value is the value to set as a return value
 *
 * @return N/A
 */
static ALWAYS_INLINE void
_set_thread_return_value(struct k_thread *thread, unsigned int value)
{
	struct __esf *esf = (struct __esf *)thread->callee_saved.psp;

	esf->a1 = value;
}

extern void nano_cpu_atomic_idle(unsigned int);

#define _is_in_isr() _IsInIsr()

extern void _IntLibInit(void);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _kernel_arch_func__h_ */
