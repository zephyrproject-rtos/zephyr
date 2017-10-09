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

/* Some configurations require that the stack/registers be adjusted before
 * _thread_entry. See discussion in swap.S for _x86_thread_entry_wrapper()
 */
#if defined(CONFIG_GDB_INFO) || defined(CONFIG_DEBUG_INFO) || \
	defined(CONFIG_X86_IAMCU)
#define WRAPPER_REQUIRED
#endif

#ifdef WRAPPER_REQUIRED
extern void _x86_thread_entry_wrapper(k_thread_entry_t entry,
				      void *p1, void *p2, void *p3);
#endif /* WRAPPER_REQUIRED */


/* Initial thread stack frame, such that everything is laid out as expected
 * for when _Swap() switches to it for the first time.
 */
struct _x86_initial_frame {
	u32_t swap_retval;
	u32_t ebp;
	u32_t ebx;
	u32_t esi;
	u32_t edi;
	void *_thread_entry;
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
void _new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		 size_t stack_size, k_thread_entry_t entry,
		 void *parameter1, void *parameter2, void *parameter3,
		 int priority, unsigned int options)
{
	char *stack_buf;
	char *stack_high;
	struct _x86_initial_frame *initial_frame;

	_ASSERT_VALID_PRIO(priority, entry);
	stack_buf = K_THREAD_STACK_BUFFER(stack);
	_new_thread_init(thread, stack_buf, stack_size, priority, options);

#if CONFIG_X86_USERSPACE
	if (!(options & K_USER)) {
		/* Running in kernel mode, kernel stack region is also a guard
		 * page */
		_x86_mmu_set_flags((void *)(stack_buf - MMU_PAGE_SIZE),
				   MMU_PAGE_SIZE, MMU_ENTRY_NOT_PRESENT,
				   MMU_PTE_P_MASK);
	}
#endif /* CONFIG_X86_USERSPACE */

#if CONFIG_X86_STACK_PROTECTION
	_x86_mmu_set_flags(stack, MMU_PAGE_SIZE, MMU_ENTRY_NOT_PRESENT,
			   MMU_PTE_P_MASK);
#endif

	stack_high = (char *)STACK_ROUND_DOWN(stack_buf + stack_size);

	/* Create an initial context on the stack expected by _Swap() */
	initial_frame = (struct _x86_initial_frame *)
		(stack_high - sizeof(struct _x86_initial_frame));
	/* _thread_entry() arguments */
	initial_frame->entry = entry;
	initial_frame->p1 = parameter1;
	initial_frame->p2 = parameter2;
	initial_frame->p3 = parameter3;
	/* initial EFLAGS; only modify IF and IOPL bits */
	initial_frame->eflags = (EflagsGet() & ~EFLAGS_MASK) | EFLAGS_INITIAL;
#ifdef CONFIG_X86_USERSPACE
	if (options & K_USER) {
#ifdef WRAPPER_REQUIRED
		initial_frame->edi = (u32_t)_arch_user_mode_enter;
		initial_frame->_thread_entry = _x86_thread_entry_wrapper;
#else
		initial_frame->_thread_entry = _arch_user_mode_enter;
#endif /* WRAPPER_REQUIRED */
	} else
#endif /* CONFIG_X86_USERSPACE */
	{
#ifdef WRAPPER_REQUIRED
		initial_frame->edi = (u32_t)_thread_entry;
		initial_frame->_thread_entry = _x86_thread_entry_wrapper;
#else
		initial_frame->_thread_entry = _thread_entry;
#endif
	}
	/* Remaining _x86_initial_frame members can be garbage, _thread_entry()
	 * doesn't care about their state when execution begins
	 */
	thread->callee_saved.esp = (unsigned long)initial_frame;

#if (defined(CONFIG_FP_SHARING) || defined(CONFIG_GDB_INFO))
	thread->arch.excNestCount = 0;
#endif /* CONFIG_FP_SHARING || CONFIG_GDB_INFO */
#ifdef CONFIG_THREAD_MONITOR
	thread->entry = (struct __thread_entry *)&initial_frame->entry;
	thread_monitor_init(thread);
#endif
}

#ifdef CONFIG_X86_USERSPACE
void _x86_swap_update_page_tables(struct k_thread *incoming,
				  struct k_thread *outgoing)
{
	/* Outgoing thread stack no longer accessible */
	_x86_mmu_set_flags((void *)outgoing->stack_info.start,
			   ROUND_UP(outgoing->stack_info.size, MMU_PAGE_SIZE),
			   MMU_ENTRY_SUPERVISOR, MMU_PTE_US_MASK);


	/* Userspace can now access the incoming thread's stack */
	_x86_mmu_set_flags((void *)incoming->stack_info.start,
			   ROUND_UP(incoming->stack_info.size, MMU_PAGE_SIZE),
			   MMU_ENTRY_USER, MMU_PTE_US_MASK);

	/* In case of privilege elevation, use the incoming thread's kernel
	 * stack, the top of the thread stack is the bottom of the kernel stack
	 */
	_main_tss.esp0 = incoming->stack_info.start;

	/* If either thread defines different memory domains, efficiently
	 * switch between them
	 */
	if (incoming->mem_domain_info.mem_domain !=
	   outgoing->mem_domain_info.mem_domain){

		 /* Ensure that the outgoing mem domain configuration
		  * is set back to default state.
		  */
		_arch_mem_domain_destroy(outgoing->mem_domain_info.mem_domain);
		_x86_mmu_mem_domain_load(incoming);
	}
}


FUNC_NORETURN void _arch_user_mode_enter(k_thread_entry_t user_entry,
					 void *p1, void *p2, void *p3)
{
	u32_t stack_end;

	/* Transition will reset stack pointer to initial, discarding
	 * any old context since this is a one-way operation
	 */
	stack_end = STACK_ROUND_DOWN(_current->stack_info.start +
				     _current->stack_info.size);

	/* Set up the kernel stack used during privilege elevation */
	_x86_mmu_set_flags((void *)(_current->stack_info.start - MMU_PAGE_SIZE),
			   MMU_PAGE_SIZE,
			   (MMU_ENTRY_PRESENT | MMU_ENTRY_WRITE |
			    MMU_ENTRY_SUPERVISOR),
			   (MMU_PTE_P_MASK | MMU_PTE_RW_MASK |
			    MMU_PTE_US_MASK));

	_x86_userspace_enter(user_entry, p1, p2, p3, stack_end,
			     _current->stack_info.start);
	CODE_UNREACHABLE;
}


/* Implemented in userspace.S */
extern void _x86_syscall_entry_stub(void);

/* Syscalls invoked by 'int 0x80'. Installed in the IDT at DPL=3 so that
 * userspace can invoke it.
 */
NANO_CPU_INT_REGISTER(_x86_syscall_entry_stub, -1, -1, 0x80, 3);

#endif /* CONFIG_X86_USERSPACE */
