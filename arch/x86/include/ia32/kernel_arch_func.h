/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* this file is only meant to be included by kernel_structs.h */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_IA32_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_X86_INCLUDE_IA32_KERNEL_ARCH_FUNC_H_

#ifndef _ASMLANGUAGE

#include <stddef.h> /* For size_t */

#ifdef __cplusplus
extern "C" {
#endif

static inline void arch_kernel_init(void)
{
	/* No-op on this arch */
}

static ALWAYS_INLINE void
arch_thread_return_value_set(struct k_thread *thread, unsigned int value)
{
	/* write into 'eax' slot created in z_swap() entry */

	*(unsigned int *)(thread->callee_saved.esp) = value;
}

extern void arch_cpu_atomic_idle(unsigned int key);

/* ASM code to fiddle with registers to enable the MMU with PAE paging */
void z_x86_enable_paging(void);

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_IA32_KERNEL_ARCH_FUNC_H_ */
