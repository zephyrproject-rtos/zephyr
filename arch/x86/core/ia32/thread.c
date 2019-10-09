/*
 * Copyright (c) 2010-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Thread support primitives
 *
 * This module provides core thread related primitives for the IA-32
 * processor architecture.
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <kernel_structs.h>
#include <wait_q.h>
#include <arch/x86/mmustructs.h>
#include <sys/printk.h>

/* forward declaration */

/* Initial thread stack frame, such that everything is laid out as expected
 * for when z_swap() switches to it for the first time.
 */
struct _x86_initial_frame {
	u32_t swap_retval;
	u32_t ebp;
	u32_t ebx;
	u32_t esi;
	u32_t edi;
	void *thread_entry;
	u32_t eflags;
	k_thread_entry_t entry;
	void *p1;
	void *p2;
	void *p3;
};

#ifdef CONFIG_X86_USERSPACE
/* Nothing to do here if KPTI is enabled. We are in supervisor mode, so the
 * active PDPT is the kernel's page tables. If the incoming thread is in user
 * mode we are going to switch CR3 to the thread- specific tables when we go
 * through z_x86_trampoline_to_user.
 *
 * We don't need to update _main_tss either, privilege elevation always lands
 * on the trampoline stack and the irq/sycall code has to manually transition
 * off of it to the thread's kernel stack after switching page tables.
 */
#ifndef CONFIG_X86_KPTI
/* Change to new set of page tables. ONLY intended for use from
 * z_x88_swap_update_page_tables(). This changes CR3, no memory access
 * afterwards is legal unless it is known for sure that the relevant
 * mappings are identical wrt supervisor mode until we iret out.
 */
static inline void page_tables_set(struct x86_page_tables *ptables)
{
	__asm__ volatile("movl %0, %%cr3\n\t" : : "r" (ptables) : "memory");
}

/* Update the to the incoming thread's page table, and update the location
 * of the privilege elevation stack.
 *
 * May be called ONLY during context switch and when supervisor
 * threads drop synchronously to user mode. Hot code path!
 */
void z_x86_swap_update_page_tables(struct k_thread *incoming)
{
	struct x86_page_tables *ptables;

	/* If we're a user thread, we want the active page tables to
	 * be the per-thread instance.
	 *
	 * However, if we're a supervisor thread, use the master
	 * kernel page tables instead.
	 */
	if ((incoming->base.user_options & K_USER) != 0) {
		ptables = z_x86_thread_page_tables_get(incoming);

		/* In case of privilege elevation, use the incoming
		 * thread's kernel stack. This area starts immediately
		 * before the PDPT.
		 */
		_main_tss.esp0 = (uintptr_t)ptables;
	} else {
		ptables = &z_x86_kernel_ptables;
	}

	/* Check first that we actually need to do this, since setting
	 * CR3 involves an expensive full TLB flush.
	 */
	if (ptables != z_x86_page_tables_get()) {
		page_tables_set(ptables);
	}
}
#endif /* CONFIG_X86_KPTI */

static FUNC_NORETURN void drop_to_user(k_thread_entry_t user_entry,
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

FUNC_NORETURN void z_arch_user_mode_enter(k_thread_entry_t user_entry,
					 void *p1, void *p2, void *p3)
{
	struct z_x86_thread_stack_header *header =
		(struct z_x86_thread_stack_header *)_current->stack_obj;

	/* Set up the kernel stack used during privilege elevation */
	z_x86_mmu_set_flags(&z_x86_kernel_ptables, &header->privilege_stack,
			    MMU_PAGE_SIZE, MMU_ENTRY_WRITE, Z_X86_MMU_RW,
			    true);

	/* Initialize per-thread page tables, since that wasn't done when
	 * the thread was initialized (K_USER was not set at creation time)
	 */
	z_x86_thread_pt_init(_current);

	/* Apply memory domain configuration, if assigned */
	if (_current->mem_domain_info.mem_domain != NULL) {
		z_x86_apply_mem_domain(z_x86_thread_page_tables_get(_current),
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
	 */
	z_x86_swap_update_page_tables(_current);
#endif

	drop_to_user(user_entry, p1, p2, p3);
}

/* Implemented in userspace.S */
extern void z_x86_syscall_entry_stub(void);

/* Syscalls invoked by 'int 0x80'. Installed in the IDT at DPL=3 so that
 * userspace can invoke it.
 */
NANO_CPU_INT_REGISTER(z_x86_syscall_entry_stub, -1, -1, 0x80, 3);

#endif /* CONFIG_X86_USERSPACE */

#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)

extern int z_float_disable(struct k_thread *thread);

int z_arch_float_disable(struct k_thread *thread)
{
#if defined(CONFIG_LAZY_FP_SHARING)
	return z_float_disable(thread);
#else
	return -ENOSYS;
#endif /* CONFIG_LAZY_FP_SHARING */
}
#endif /* CONFIG_FLOAT && CONFIG_FP_SHARING */

void z_arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		       size_t stack_size, k_thread_entry_t entry,
		       void *parameter1, void *parameter2, void *parameter3,
		       int priority, unsigned int options)
{
	char *stack_buf;
	char *stack_high;
	struct _x86_initial_frame *initial_frame;
#if defined(CONFIG_X86_USERSPACE) || defined(CONFIG_X86_STACK_PROTECTION)
	struct z_x86_thread_stack_header *header =
		(struct z_x86_thread_stack_header *)stack;
#endif

	Z_ASSERT_VALID_PRIO(priority, entry);
	stack_buf = Z_THREAD_STACK_BUFFER(stack);
	z_new_thread_init(thread, stack_buf, stack_size, priority, options);

#ifdef CONFIG_X86_USERSPACE
	/* Set MMU properties for the privilege mode elevation stack.
	 * If we're not starting in user mode, this functions as a guard
	 * area.
	 */
	z_x86_mmu_set_flags(&z_x86_kernel_ptables, &header->privilege_stack,
		MMU_PAGE_SIZE,
		((options & K_USER) == 0U) ? MMU_ENTRY_READ : MMU_ENTRY_WRITE,
		Z_X86_MMU_RW, true);
#endif /* CONFIG_X86_USERSPACE */

#if CONFIG_X86_STACK_PROTECTION
	/* Set guard area to read-only to catch stack overflows */
	z_x86_mmu_set_flags(&z_x86_kernel_ptables, &header->guard_page,
			    MMU_PAGE_SIZE, MMU_ENTRY_READ, Z_X86_MMU_RW,
			    true);
#endif

	stack_high = (char *)STACK_ROUND_DOWN(stack_buf + stack_size);

	/* Create an initial context on the stack expected by z_swap() */
	initial_frame = (struct _x86_initial_frame *)
		(stack_high - sizeof(struct _x86_initial_frame));
	/* z_thread_entry() arguments */
	initial_frame->entry = entry;
	initial_frame->p1 = parameter1;
	initial_frame->p2 = parameter2;
	initial_frame->p3 = parameter3;
	initial_frame->eflags = EFLAGS_INITIAL;
#ifdef CONFIG_X86_USERSPACE
	if ((options & K_USER) != 0U) {
		z_x86_thread_pt_init(thread);
#ifdef _THREAD_WRAPPER_REQUIRED
		initial_frame->edi = (u32_t)drop_to_user;
		initial_frame->thread_entry = z_x86_thread_entry_wrapper;
#else
		initial_frame->thread_entry = drop_to_user;
#endif /* _THREAD_WRAPPER_REQUIRED */
	} else
#endif /* CONFIG_X86_USERSPACE */
	{
#ifdef _THREAD_WRAPPER_REQUIRED
		initial_frame->edi = (u32_t)z_thread_entry;
		initial_frame->thread_entry = z_x86_thread_entry_wrapper;
#else
		initial_frame->thread_entry = z_thread_entry;
#endif
	}
	/* Remaining _x86_initial_frame members can be garbage, z_thread_entry()
	 * doesn't care about their state when execution begins
	 */
	thread->callee_saved.esp = (unsigned long)initial_frame;

#if defined(CONFIG_LAZY_FP_SHARING)
	thread->arch.excNestCount = 0;
#endif /* CONFIG_LAZY_FP_SHARING */

	thread->arch.flags = 0;
}
