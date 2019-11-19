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

#ifndef CONFIG_X86_KPTI
/* Change to new set of page tables. ONLY intended for use from
 * z_x88_swap_update_page_tables(). This changes CR3, no memory access
 * afterwards is legal unless it is known for sure that the relevant
 * mappings are identical wrt supervisor mode until we iret out.
 */
static inline void page_tables_set(struct x86_page_tables *ptables)
{
#ifdef CONFIG_X86_64
	__asm__ volatile("movq %0, %%cr3\n\t" : : "r" (ptables) : "memory");
#else
	__asm__ volatile("movl %0, %%cr3\n\t" : : "r" (ptables) : "memory");
#endif
}

/* Set initial stack pointer for privilege mode elevations */
static inline void set_initial_psp(char *psp)
{
#ifdef CONFIG_X86_64
	__asm__ volatile("movq %0, %%gs:__x86_tss64_t_psp_OFFSET\n\t"
			 : : "r" (psp));
#else
	_main_tss.esp0 = (uintptr_t)psp;
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
 * code has to manually transition off of it to the thread's kernel stack after
 * switching page tables.
 */
void z_x86_swap_update_page_tables(struct k_thread *incoming)
{
	struct x86_page_tables *ptables;

	if ((incoming->base.user_options & K_USER) != 0) {
		set_initial_psp(incoming->arch.psp);
	}

	/* Check first that we actually need to do this, since setting
	 * CR3 involves an expensive full TLB flush.
	 */
	ptables = z_x86_thread_page_tables_get(incoming);

	if (ptables != z_x86_page_tables_get()) {
		page_tables_set(ptables);
	}
}
#endif /* CONFIG_X86_KPTI */

FUNC_NORETURN static void drop_to_user(k_thread_entry_t user_entry,
				       void *p1, void *p2, void *p3)
{
	u32_t stack_end;

	/* Transition will reset stack pointer to initial, discarding
	 * any old context since this is a one-way operation
	 */
	stack_end = STACK_ROUND_DOWN(_current->stack_info.start +
				     _current->stack_info.size);

	z_x86_userspace_enter(user_entry, p1, p2, p3, stack_end,
			      _current->stack_info.start);
	CODE_UNREACHABLE;
}

static inline void
set_privilege_stack_perms(struct z_x86_thread_stack_header *header,
			  bool is_usermode)
{
	/* Set MMU properties for the privilege mode elevation stack. If we're
	 * not in user mode, this functions as a guard area.
	 */
	z_x86_mmu_set_flags(&z_x86_kernel_ptables, &header->privilege_stack,
			    MMU_PAGE_SIZE,
			    is_usermode ? MMU_ENTRY_WRITE : MMU_ENTRY_READ,
			    Z_X86_MMU_RW, true);
}

/* Does the following:
 *
 * - Allows the kernel to write to the privilege elevation stack area.
 * - Initialize per-thread page tables and update thread->arch.ptables to
 *   point to them.
 * - Set thread->arch.psp to point to the initial stack pointer for user
 *   mode privilege elevation for system calls; supervisor mode threads leave
 *   this uninitailized.
 */
static void prepare_user_thread(struct k_thread *thread)
{
	struct z_x86_thread_stack_header *header =
		(struct z_x86_thread_stack_header *)thread->stack_obj;

	__ASSERT((thread->base.user_options & K_USER) != 0,
		 "not a user thread");

	/* Set privileve elevation stack area to writable. Need to do this
	 * before calling z_x86_pt_init(), as on 32-bit the top-level PDPT
	 * is in there as well.
	 */
	set_privilege_stack_perms(header, true);

	/* Create and program into the MMU the per-thread page tables */
	z_x86_thread_pt_init(thread);

	thread->arch.psp =
		header->privilege_stack + sizeof(header->privilege_stack);
}

static void prepare_supervisor_thread(struct k_thread *thread)
{
	struct z_x86_thread_stack_header *header =
		(struct z_x86_thread_stack_header *)thread->stack_obj;

	thread->arch.ptables = &z_x86_kernel_ptables;

	/* Privilege elevation stack set to read-only to function
	 * as a guard area. This gets made writable if we drop
	 * to user mode later.
	 */
	set_privilege_stack_perms(header, false);
}

/* Preparation steps needed for all threads if user mode is turned on.
 *
 * Returns the initial entry point to swap into.
 */
void *z_x86_userspace_prepare_thread(struct k_thread *thread)
{
	void *initial_entry;

	if ((thread->base.user_options & K_USER) != 0U) {
		prepare_user_thread(thread);
		initial_entry = drop_to_user;
	} else {
		prepare_supervisor_thread(thread);
		initial_entry = z_thread_entry;
	}

	return initial_entry;
}

FUNC_NORETURN void arch_user_mode_enter(k_thread_entry_t user_entry,
					void *p1, void *p2, void *p3)
{
	prepare_user_thread(_current);

	/* Apply memory domain configuration, if assigned. Threads that
	 * started in user mode already had this done via z_setup_new_thread()
	 */
	if (_current->mem_domain_info.mem_domain != NULL) {
		z_x86_apply_mem_domain(_current->arch.ptables,
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
