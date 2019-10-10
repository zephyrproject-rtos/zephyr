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

/* stack alignment related macros: STACK_ALIGN_SIZE is defined above */

#define STACK_ROUND_UP(x) ROUND_UP(x, STACK_ALIGN_SIZE)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, STACK_ALIGN_SIZE)

static inline void z_arch_kernel_init(void)
{
	/* No-op on this arch */
}

static ALWAYS_INLINE void
z_arch_thread_return_value_set(struct k_thread *thread, unsigned int value)
{
	/* write into 'eax' slot created in z_swap() entry */

	*(unsigned int *)(thread->callee_saved.esp) = value;
}

extern void z_arch_cpu_atomic_idle(unsigned int key);

#ifdef CONFIG_USERSPACE
extern FUNC_NORETURN void z_x86_userspace_enter(k_thread_entry_t user_entry,
					       void *p1, void *p2, void *p3,
					       u32_t stack_end,
					       u32_t stack_start);

void z_x86_thread_pt_init(struct k_thread *thread);

void z_x86_apply_mem_domain(struct x86_page_tables *ptables,
			    struct k_mem_domain *mem_domain);

static inline struct x86_page_tables *
z_x86_thread_page_tables_get(struct k_thread *thread)
{
	struct z_x86_thread_stack_header *header =
		(struct z_x86_thread_stack_header *)thread->stack_obj;

	return &header->kernel_data.ptables;
}
#endif /* CONFIG_USERSPACE */

/* ASM code to fiddle with registers to enable the MMU with PAE paging */
void z_x86_enable_paging(void);

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_IA32_KERNEL_ARCH_FUNC_H_ */
