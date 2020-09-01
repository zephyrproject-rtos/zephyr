/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/speculation.h>
#include <syscall_handler.h>
#include <kernel_arch_func.h>
#include <ksched.h>
#include <x86_mmu.h>

#ifndef CONFIG_X86_KPTI
/* Set CR3 to a physical address. There must be a valid top-level paging
 * structure here or the CPU will triple fault. The incoming page tables must
 * have the same kernel mappings wrt supervisor mode. Don't use this function
 * unless you know exactly what you are doing.
 */
static inline void cr3_set(uintptr_t phys)
{
#ifdef CONFIG_X86_64
	__asm__ volatile("movq %0, %%cr3\n\t" : : "r" (phys) : "memory");
#else
	__asm__ volatile("movl %0, %%cr3\n\t" : : "r" (phys) : "memory");
#endif
}

/* Update the to the incoming thread's page table, and update the location of
 * the privilege elevation stack.
 *
 * May be called ONLY during context switch and when supervisor threads drop
 * synchronously to user mode. Hot code path!
 *
 * Nothing to do here if KPTI is enabled. We are in supervisor mode, so the
 * active page tables are the kernel's page tables. If the incoming thread is
 * in user mode we are going to switch CR3 to the thread-specific tables when
 * we go through z_x86_trampoline_to_user.
 *
 * We don't need to update the privilege mode initial stack pointer either,
 * privilege elevation always lands on the trampoline stack and the irq/sycall
 * code has to manually transition off of it to the appropriate stack after
 * switching page tables.
 */
void z_x86_swap_update_page_tables(struct k_thread *incoming)
{
	uintptr_t ptables_phys;

#ifndef CONFIG_X86_64
	/* 64-bit uses syscall/sysret which switches stacks manually,
	 * tss64.psp is updated unconditionally in __resume
	 */
	if ((incoming->base.user_options & K_USER) != 0) {
		_main_tss.esp0 = (uintptr_t)incoming->arch.psp;
	}
#endif

	/* Check first that we actually need to do this, since setting
	 * CR3 involves an expensive full TLB flush.
	 */
	ptables_phys = incoming->arch.ptables;

	if (ptables_phys != z_x86_cr3_get()) {
		cr3_set(ptables_phys);
	}
}
#endif /* CONFIG_X86_KPTI */

FUNC_NORETURN static void drop_to_user(k_thread_entry_t user_entry,
				       void *p1, void *p2, void *p3)
{
	uint32_t stack_end;

	/* Transition will reset stack pointer to initial, discarding
	 * any old context since this is a one-way operation
	 */
	stack_end = Z_STACK_PTR_ALIGN(_current->stack_info.start +
				      _current->stack_info.size -
				      _current->stack_info.delta);

	z_x86_userspace_enter(user_entry, p1, p2, p3, stack_end,
			      _current->stack_info.start);
	CODE_UNREACHABLE;
}

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

	if ((thread->base.user_options & K_USER) != 0U) {
		z_x86_thread_pt_init(thread);
		initial_entry = drop_to_user;
	} else {
		thread->arch.ptables = (uintptr_t)&z_x86_kernel_ptables;
		initial_entry = z_thread_entry;
	}

	return initial_entry;
}

FUNC_NORETURN void arch_user_mode_enter(k_thread_entry_t user_entry,
					void *p1, void *p2, void *p3)
{
	z_x86_thread_pt_init(_current);

	/* Apply memory domain configuration, if assigned. Threads that
	 * started in user mode already had this done via z_setup_new_thread()
	 */
	if (_current->mem_domain_info.mem_domain != NULL) {
		z_x86_apply_mem_domain(_current,
				       _current->mem_domain_info.mem_domain);
	}

#ifndef CONFIG_X86_KPTI
	/* We're synchronously dropping into user mode from a thread that
	 * used to be in supervisor mode. K_USER flag has now been set, but
	 * Need to swap from the kernel's page tables to the per-thread page
	 * tables.
	 *
	 * Safe to update page tables from here, all tables are identity-
	 * mapped and memory areas used before the ring 3 transition all
	 * have the same attributes wrt supervisor mode access.
	 *
	 * Threads that started in user mode already had this applied on
	 * initial context switch.
	 */
	z_x86_swap_update_page_tables(_current);
#endif

	drop_to_user(user_entry, p1, p2, p3);
}
