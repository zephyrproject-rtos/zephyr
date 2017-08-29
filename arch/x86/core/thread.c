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

/* forward declaration */

#if defined(CONFIG_GDB_INFO) || defined(CONFIG_DEBUG_INFO) \
	|| defined(CONFIG_X86_IAMCU)
extern void _thread_entry_wrapper(_thread_entry_t entry,
				  void *p1, void *p2, void *p3);
/**
 *
 * @brief Adjust stack/parameters before invoking _thread_entry
 *
 * This function adjusts the initial stack frame created by _new_thread() such
 * that the GDB stack frame unwinders recognize it as the outermost frame in
 * the thread's stack.  For targets that use the IAMCU calling convention, the
 * first three arguments are popped into eax, edx, and ecx. The function then
 * jumps to _thread_entry().
 *
 * GDB normally stops unwinding a stack when it detects that it has
 * reached a function called main().  Kernel tasks, however, do not have
 * a main() function, and there does not appear to be a simple way of stopping
 * the unwinding of the stack.
 *
 * SYS V Systems:
 *
 * Given the initial thread created by _new_thread(), GDB expects to find a
 * return address on the stack immediately above the thread entry routine
 * _thread_entry, in the location occupied by the initial EFLAGS.
 * GDB attempts to examine the memory at this return address, which typically
 * results in an invalid access to page 0 of memory.
 *
 * This function overwrites the initial EFLAGS with zero.  When GDB subsequently
 * attempts to examine memory at address zero, the PeekPoke driver detects
 * an invalid access to address zero and returns an error, which causes the
 * GDB stack unwinder to stop somewhat gracefully.
 *
 * The initial EFLAGS cannot be overwritten until after _Swap() has swapped in
 * the new thread for the first time.  This routine is called by _Swap() the
 * first time that the new thread is swapped in, and it jumps to
 * _thread_entry after it has done its work.
 *
 * IAMCU Systems:
 *
 * There is no EFLAGS on the stack when we get here. _thread_entry() takes
 * four arguments, and we need to pop off the first three into the
 * appropriate registers. Instead of using the 'call' instruction, we push
 * a NULL return address onto the stack and jump into _thread_entry,
 * ensuring the stack won't be unwound further. Placing some kind of return
 * address on the stack is mandatory so this isn't conditionally compiled.
 *
 *       __________________
 *      |      param3      |   <------ Top of the stack
 *      |__________________|
 *      |      param2      |           Stack Grows Down
 *      |__________________|                  |
 *      |      param1      |                  V
 *      |__________________|
 *      |      pEntry      |  <----   ESP when invoked by _Swap() on IAMCU
 *      |__________________|
 *      | initial EFLAGS   |  <----   ESP when invoked by _Swap() on Sys V
 *      |__________________|             (Zeroed by this routine on Sys V)
 *
 *
 *
 * @return this routine does NOT return.
 */
__asm__("\t.globl _thread_entry\n"
	"\t.section .text\n"
	"_thread_entry_wrapper:\n" /* should place this func .S file and use
				    * SECTION_FUNC
				    */

#ifdef CONFIG_X86_IAMCU
	/* IAMCU calling convention has first 3 arguments supplied in
	 * registers not the stack
	 */
	"\tpopl %eax\n"
	"\tpopl %edx\n"
	"\tpopl %ecx\n"
	"\tpushl $0\n" /* Null return address */
#elif defined(CONFIG_GDB_INFO) || defined(CONFIG_DEBUG_INFO)
	"\tmovl $0, (%esp)\n" /* zero initialEFLAGS location */
#endif
	"\tjmp _thread_entry\n");
#endif /* CONFIG_GDB_INFO || CONFIG_DEBUG_INFO) || CONFIG_X86_IAMCU */

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
	_thread_entry_t entry;
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
void _new_thread(struct k_thread *thread, k_thread_stack_t stack,
		 size_t stack_size, _thread_entry_t entry,
		 void *parameter1, void *parameter2, void *parameter3,
		 int priority, unsigned int options)
{
	char *stack_buf;
	char *stack_high;
	struct _x86_initial_frame *initial_frame;

	_ASSERT_VALID_PRIO(priority, entry);
#if CONFIG_X86_STACK_PROTECTION
	_x86_mmu_set_flags(stack, MMU_PAGE_SIZE, MMU_ENTRY_NOT_PRESENT,
			   MMU_PTE_P_MASK);
#endif
	stack_buf = K_THREAD_STACK_BUFFER(stack);
	_new_thread_init(thread, stack_buf, stack_size, priority, options);

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
#if defined(CONFIG_GDB_INFO) || defined(CONFIG_DEBUG_INFO) \
	|| defined(CONFIG_X86_IAMCU)
	 /* Adjust the stack before _thread_entry() is invoked */
	initial_frame->_thread_entry = _thread_entry_wrapper;
#else
	initial_frame->_thread_entry = _thread_entry;
#endif
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
