/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_X86_INCLUDE_KERNEL_ARCH_FUNC_H_

#include <kernel_arch_data.h>
#include <arch/x86/mmustructs.h>

#ifdef CONFIG_X86_64
#include <intel64/kernel_arch_func.h>
#else
#include <ia32/kernel_arch_func.h>
#endif

#ifndef _ASMLANGUAGE
static inline bool arch_is_in_isr(void)
{
#ifdef CONFIG_SMP
	/* On SMP, there is a race vs. the current CPU changing if we
	 * are preempted.  Need to mask interrupts while inspecting
	 * (note deliberate lack of gcc size suffix on the
	 * instructions, we need to work with both architectures here)
	 */
	bool ret;

	__asm__ volatile ("pushf; cli");
	ret = arch_curr_cpu()->nested != 0;
	__asm__ volatile ("popf");
	return ret;
#else
	return _kernel.cpus[0].nested != 0U;
#endif
}

struct multiboot_info;

extern FUNC_NORETURN void z_x86_prep_c(void *arg);

#ifdef CONFIG_X86_VERY_EARLY_CONSOLE
/* Setup ultra-minimal serial driver for printk() */
void z_x86_early_serial_init(void);
#endif /* CONFIG_X86_VERY_EARLY_CONSOLE */


/* Called upon CPU exception that is unhandled and hence fatal; dump
 * interesting info and call z_x86_fatal_error()
 */
FUNC_NORETURN void z_x86_unhandled_cpu_exception(uintptr_t vector,
						 const z_arch_esf_t *esf);

/* Called upon unrecoverable error; dump registers and transfer control to
 * kernel via z_fatal_error()
 */
FUNC_NORETURN void z_x86_fatal_error(unsigned int reason,
				     const z_arch_esf_t *esf);

/* Common handling for page fault exceptions */
void z_x86_page_fault_handler(z_arch_esf_t *esf);

#ifdef CONFIG_THREAD_STACK_INFO
/**
 * @brief Check if a memory address range falls within the stack
 *
 * Given a memory address range, ensure that it falls within the bounds
 * of the faulting context's stack.
 *
 * @param addr Starting address
 * @param size Size of the region, or 0 if we just want to see if addr is
 *             in bounds
 * @param cs Code segment of faulting context
 * @return true if addr/size region is not within the thread stack
 */
bool z_x86_check_stack_bounds(uintptr_t addr, size_t size, uint16_t cs);
#endif /* CONFIG_THREAD_STACK_INFO */

#ifdef CONFIG_USERSPACE
extern FUNC_NORETURN void z_x86_userspace_enter(k_thread_entry_t user_entry,
					       void *p1, void *p2, void *p3,
					       uintptr_t stack_end,
					       uintptr_t stack_start);

/* Preparation steps needed for all threads if user mode is turned on.
 *
 * Returns the initial entry point to swap into.
 */
void *z_x86_userspace_prepare_thread(struct k_thread *thread);

#endif /* CONFIG_USERSPACE */

void z_x86_do_kernel_oops(const z_arch_esf_t *esf);

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_KERNEL_ARCH_FUNC_H_ */
