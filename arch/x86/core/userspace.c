/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/speculation.h>
#include <zephyr/internal/syscall_handler.h>
#include <kernel_arch_func.h>
#include <ksched.h>
#include <x86_mmu.h>

BUILD_ASSERT((CONFIG_PRIVILEGED_STACK_SIZE > 0) &&
	     (CONFIG_PRIVILEGED_STACK_SIZE % CONFIG_MMU_PAGE_SIZE) == 0);

#ifdef CONFIG_DEMAND_PAGING
#include <zephyr/kernel/mm/demand_paging.h>
#endif

#ifndef CONFIG_X86_KPTI
/* Update the to the incoming thread's page table, and update the location of
 * the privilege elevation stack.
 *
 * May be called ONLY during context switch. Hot code path!
 *
 * Nothing to do here if KPTI is enabled. We are in supervisor mode, so the
 * active page tables are the kernel's page tables. If the incoming thread is
 * in user mode we are going to switch CR3 to the domain-specific tables when
 * we go through z_x86_trampoline_to_user.
 *
 * We don't need to update the privilege mode initial stack pointer either,
 * privilege elevation always lands on the trampoline stack and the irq/syscall
 * code has to manually transition off of it to the appropriate stack after
 * switching page tables.
 */
__pinned_func
void z_x86_swap_update_page_tables(struct k_thread *incoming)
{
#ifndef CONFIG_X86_64
	/* Set initial stack pointer when elevating privileges from Ring 3
	 * to Ring 0.
	 */
	_main_tss.esp0 = (uintptr_t)incoming->arch.psp;
#endif

#ifdef CONFIG_X86_COMMON_PAGE_TABLE
	z_x86_swap_update_common_page_table(incoming);
#else
	/* Check first that we actually need to do this, since setting
	 * CR3 involves an expensive full TLB flush.
	 */
	uintptr_t ptables_phys = incoming->arch.ptables;

	__ASSERT(ptables_phys != 0, "NULL page tables for thread %p\n",
		 incoming);

	if (ptables_phys != z_x86_cr3_get()) {
		z_x86_cr3_set(ptables_phys);
	}
#endif /* CONFIG_X86_COMMON_PAGE_TABLE */
}
#endif /* CONFIG_X86_KPTI */

/* Preparation steps needed for all threads if user mode is turned on.
 *
 * Returns the initial entry point to swap into.
 */
void *z_x86_userspace_prepare_thread(struct k_thread *thread)
{
	void *initial_entry;
	struct z_x86_thread_stack_header *header =
		(struct z_x86_thread_stack_header *)thread->stack_obj;

	thread->arch.psp =
		header->privilege_stack + sizeof(header->privilege_stack);

#ifndef CONFIG_X86_COMMON_PAGE_TABLE
	/* Important this gets cleared, so that arch_mem_domain_* APIs
	 * can distinguish between new threads, and threads migrating
	 * between domains
	 */
	thread->arch.ptables = (uintptr_t)NULL;
#endif /* CONFIG_X86_COMMON_PAGE_TABLE */

	if ((thread->base.user_options & K_USER) != 0U) {
		initial_entry = arch_user_mode_enter;
	} else {
		initial_entry = z_thread_entry;
	}

	return initial_entry;
}

FUNC_NORETURN void arch_user_mode_enter(k_thread_entry_t user_entry,
					void *p1, void *p2, void *p3)
{
	size_t stack_end;

	/* Transition will reset stack pointer to initial, discarding
	 * any old context since this is a one-way operation
	 */
	stack_end = Z_STACK_PTR_ALIGN(_current->stack_info.start +
				      _current->stack_info.size -
				      _current->stack_info.delta);

#ifdef CONFIG_X86_64
	/* x86_64 SysV ABI requires 16 byte stack alignment, which
	 * means that on entry to a C function (which follows a CALL
	 * that pushes 8 bytes) the stack must be MISALIGNED by
	 * exactly 8 bytes.
	 */
	stack_end -= 8;
#endif

#if defined(CONFIG_DEMAND_PAGING) && \
	!defined(CONFIG_LINKER_GENERIC_SECTIONS_PRESENT_AT_BOOT)
	/* If generic section is not present at boot,
	 * the thread stack may not be in physical memory.
	 * Unconditionally page in the stack instead of
	 * relying on page fault to speed up a little bit
	 * on starting the thread.
	 *
	 * Note that this also needs to page in the reserved
	 * portion of the stack (which is usually the page just
	 * before the beginning of stack in
	 * _current->stack_info.start.
	 */
	uintptr_t stack_start;
	size_t stack_size;
	uintptr_t stack_aligned_start;
	size_t stack_aligned_size;

	stack_start = POINTER_TO_UINT(_current->stack_obj);
	stack_size = K_THREAD_STACK_LEN(_current->stack_info.size);

#if defined(CONFIG_HW_STACK_PROTECTION)
	/* With hardware stack protection, the first page of stack
	 * is a guard page. So need to skip it.
	 */
	stack_start += CONFIG_MMU_PAGE_SIZE;
	stack_size -= CONFIG_MMU_PAGE_SIZE;
#endif

	(void)k_mem_region_align(&stack_aligned_start, &stack_aligned_size,
				 stack_start, stack_size,
				 CONFIG_MMU_PAGE_SIZE);
	k_mem_page_in(UINT_TO_POINTER(stack_aligned_start),
		      stack_aligned_size);
#endif

	z_x86_userspace_enter(user_entry, p1, p2, p3, stack_end,
			      _current->stack_info.start);
	CODE_UNREACHABLE;
}
