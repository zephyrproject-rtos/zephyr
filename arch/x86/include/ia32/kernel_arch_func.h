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

#ifdef __cplusplus
extern "C" {
#endif

/* stack alignment related macros: STACK_ALIGN_SIZE is defined above */

#define STACK_ROUND_UP(x) ROUND_UP(x, STACK_ALIGN_SIZE)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, STACK_ALIGN_SIZE)

extern K_THREAD_STACK_DEFINE(_interrupt_stack, CONFIG_ISR_STACK_SIZE);

#ifdef CONFIG_X86_VERY_EARLY_CONSOLE
/* Setup ultra-minimal serial driver for printk() */
void z_x86_early_serial_init(void);
#endif

/* Create all page tables with boot configuration and enable paging */
void z_x86_paging_init(void);

/**
 *
 * @brief Performs architecture-specific initialization
 *
 * This routine performs architecture-specific initialization of the kernel.
 * Trivial stuff is done inline; more complex initialization is done via
 * function calls.
 *
 * @return N/A
 */
static inline void kernel_arch_init(void)
{
	_kernel.nested = 0;
	_kernel.irq_stack = Z_THREAD_STACK_BUFFER(_interrupt_stack) +
				CONFIG_ISR_STACK_SIZE;

#ifdef CONFIG_X86_VERY_EARLY_CONSOLE
	z_x86_early_serial_init();
#endif
#ifdef CONFIG_X86_MMU
	z_x86_paging_init();
#endif
#if CONFIG_X86_STACK_PROTECTION
	z_x86_mmu_set_flags(&z_x86_kernel_pdpt, _interrupt_stack, MMU_PAGE_SIZE,
			    MMU_ENTRY_READ, MMU_PTE_RW_MASK, true);
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
 * is set to @a value.  It is assumed that the specified @a thread is pending, and
 * thus the threads context is stored in its TCS.
 *
 * @return N/A
 */
static ALWAYS_INLINE void
z_set_thread_return_value(struct k_thread *thread, unsigned int value)
{
	/* write into 'eax' slot created in z_swap() entry */

	*(unsigned int *)(thread->callee_saved.esp) = value;
}

extern void k_cpu_atomic_idle(unsigned int key);

#ifdef CONFIG_USERSPACE
extern FUNC_NORETURN void z_x86_userspace_enter(k_thread_entry_t user_entry,
					       void *p1, void *p2, void *p3,
					       u32_t stack_end,
					       u32_t stack_start);

void z_x86_thread_pt_init(struct k_thread *thread);

void z_x86_apply_mem_domain(struct x86_mmu_pdpt *pdpt,
			    struct k_mem_domain *mem_domain);

static inline struct x86_mmu_pdpt *z_x86_pdpt_get(struct k_thread *thread)
{
	struct z_x86_thread_stack_header *header =
		(struct z_x86_thread_stack_header *)thread->stack_obj;

	return &header->kernel_data.pdpt;
}
#endif /* CONFIG_USERSPACE */

/* ASM code to fiddle with registers to enable the MMU with PAE paging */
void z_x86_enable_paging(void);

#include <stddef.h> /* For size_t */

#ifdef __cplusplus
}
#endif

#define z_is_in_isr() (_kernel.nested != 0U)

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_IA32_KERNEL_ARCH_FUNC_H_ */
