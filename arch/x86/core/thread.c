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

#ifdef CONFIG_INIT_STACKS
#include <string.h>
#endif /* CONFIG_INIT_STACKS */

#include <toolchain.h>
#include <linker/sections.h>
#include <kernel_structs.h>
#include <wait_q.h>
#include <mmustructs.h>
#include <misc/printk.h>

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

/**
 * @brief Create a new kernel execution thread
 *
 * Initializes the k_thread object and sets up initial stack frame.
 *
 * @param thread pointer to thread struct memory, including any space needed
 *		for extra coprocessor context
 * @param stack the pointer to aligned stack memory
 * @param stack_size the stack size in bytes
 * @param entry thread entry point routine
 * @param parameter1 first param to entry point
 * @param parameter2 second param to entry point
 * @param parameter3 third param to entry point
 * @param priority thread priority
 * @param options thread options: K_ESSENTIAL, K_FP_REGS, K_SSE_REGS
 */
void z_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		 size_t stack_size, k_thread_entry_t entry,
		 void *parameter1, void *parameter2, void *parameter3,
		 int priority, unsigned int options)
{
	char *stack_buf;
	char *stack_high;
	struct _x86_initial_frame *initial_frame;

	Z_ASSERT_VALID_PRIO(priority, entry);
	stack_buf = Z_THREAD_STACK_BUFFER(stack);
	z_new_thread_init(thread, stack_buf, stack_size, priority, options);

#if CONFIG_X86_USERSPACE
	if ((options & K_USER) == 0U) {
		/* Running in kernel mode, kernel stack region is also a guard
		 * page */
		z_x86_mmu_set_flags(&z_x86_kernel_pdpt,
				   (void *)(stack_buf - MMU_PAGE_SIZE),
				   MMU_PAGE_SIZE, MMU_ENTRY_NOT_PRESENT,
				   MMU_PTE_P_MASK);
	}
#endif /* CONFIG_X86_USERSPACE */

#if CONFIG_X86_STACK_PROTECTION
	z_x86_mmu_set_flags(&z_x86_kernel_pdpt, stack, MMU_PAGE_SIZE,
			   MMU_ENTRY_NOT_PRESENT, MMU_PTE_P_MASK);
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
	/* initial EFLAGS; only modify IF and IOPL bits */
	initial_frame->eflags = (EflagsGet() & ~EFLAGS_MASK) | EFLAGS_INITIAL;
#ifdef CONFIG_X86_USERSPACE
	if ((options & K_USER) != 0U) {
#ifdef _THREAD_WRAPPER_REQUIRED
		initial_frame->edi = (u32_t)z_arch_user_mode_enter;
		initial_frame->thread_entry = z_x86_thread_entry_wrapper;
#else
		initial_frame->thread_entry = z_arch_user_mode_enter;
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
}

#ifdef CONFIG_X86_USERSPACE
void _x86_swap_update_page_tables(struct k_thread *incoming,
				  struct k_thread *outgoing)
{
	/* Outgoing thread stack no longer accessible */
	z_x86_reset_pages((void *)outgoing->stack_info.start,
			   ROUND_UP(outgoing->stack_info.size, MMU_PAGE_SIZE));

	/* Userspace can now access the incoming thread's stack */
	z_x86_mmu_set_flags(&USER_PDPT,
			   (void *)incoming->stack_info.start,
			   ROUND_UP(incoming->stack_info.size, MMU_PAGE_SIZE),
			   MMU_ENTRY_PRESENT | K_MEM_PARTITION_P_RW_U_RW,
			   K_MEM_PARTITION_PERM_MASK | MMU_PTE_P_MASK);

#ifndef CONFIG_X86_KPTI
	/* In case of privilege elevation, use the incoming thread's kernel
	 * stack, the top of the thread stack is the bottom of the kernel
	 * stack.
	 *
	 * If KPTI is enabled, then privilege elevation always lands on the
	 * trampoline stack and the irq/sycall code has to manually transition
	 * off of it to the thread's kernel stack after switching page
	 * tables.
	 */
	_main_tss.esp0 = incoming->stack_info.start;
#endif

	/* If either thread defines different memory domains, efficiently
	 * switch between them
	 */
	if (incoming->mem_domain_info.mem_domain !=
	   outgoing->mem_domain_info.mem_domain){

		 /* Ensure that the outgoing mem domain configuration
		  * is set back to default state.
		  */
		z_arch_mem_domain_destroy(outgoing->mem_domain_info.mem_domain);
		z_arch_mem_domain_configure(incoming);
	}
}


FUNC_NORETURN void z_arch_user_mode_enter(k_thread_entry_t user_entry,
					 void *p1, void *p2, void *p3)
{
	u32_t stack_end;

	/* Transition will reset stack pointer to initial, discarding
	 * any old context since this is a one-way operation
	 */
	stack_end = STACK_ROUND_DOWN(_current->stack_info.start +
				     _current->stack_info.size);

	/* Set up the kernel stack used during privilege elevation */
	z_x86_mmu_set_flags(&z_x86_kernel_pdpt,
			   (void *)(_current->stack_info.start - MMU_PAGE_SIZE),
			   MMU_PAGE_SIZE,
			   (MMU_ENTRY_PRESENT | MMU_ENTRY_WRITE |
			    MMU_ENTRY_SUPERVISOR),
			   (MMU_PTE_P_MASK | MMU_PTE_RW_MASK |
			    MMU_PTE_US_MASK));

	z_x86_userspace_enter(user_entry, p1, p2, p3, stack_end,
			     _current->stack_info.start);
	CODE_UNREACHABLE;
}


/* Implemented in userspace.S */
extern void z_x86_syscall_entry_stub(void);

/* Syscalls invoked by 'int 0x80'. Installed in the IDT at DPL=3 so that
 * userspace can invoke it.
 */
NANO_CPU_INT_REGISTER(z_x86_syscall_entry_stub, -1, -1, 0x80, 3);

#endif /* CONFIG_X86_USERSPACE */
