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
#include <sections.h>
#include <kernel_structs.h>
#include <wait_q.h>

/* forward declaration */

#if defined(CONFIG_GDB_INFO) || defined(CONFIG_DEBUG_INFO) \
	|| defined(CONFIG_X86_IAMCU)
void _thread_entry_wrapper(_thread_entry_t, void *,
			   void *, void *);
#endif

#if defined(CONFIG_THREAD_MONITOR)
/*
 * Add a thread to the kernel's list of active threads.
 */
static ALWAYS_INLINE void thread_monitor_init(struct k_thread *thread)
{
	unsigned int key;

	key = irq_lock();
	thread->next_thread = _kernel.threads;
	_kernel.threads = thread;
	irq_unlock(key);
}
#else
#define thread_monitor_init(thread) \
	do {/* do nothing */     \
	} while ((0))
#endif /* CONFIG_THREAD_MONITOR */

/**
 *
 * @brief Initialize a new execution thread
 *
 * This function is utilized to initialize all execution threads (both fiber
 * and task).  The 'priority' parameter will be set to -1 for the creation of
 * task.
 *
 * This function is called by _new_thread() to initialize tasks.
 *
 * @param pStackMem pointer to thread stack memory
 * @param stackSize size of a stack in bytes
 * @param priority thread priority
 * @param options thread options: K_ESSENTIAL, K_FP_REGS, K_SSE_REGS
 *
 * @return N/A
 */
static void _new_thread_internal(char *pStackMem, unsigned stackSize,
				 int priority,
				 unsigned options)
{
	unsigned long *pInitialCtx;
	/* ptr to the new task's k_thread */
	struct k_thread *thread = (struct k_thread *)pStackMem;

#if (defined(CONFIG_FP_SHARING) || defined(CONFIG_GDB_INFO))
	thread->arch.excNestCount = 0;
#endif /* CONFIG_FP_SHARING || CONFIG_GDB_INFO */

	_init_thread_base(&thread->base, priority, _THREAD_PRESTART, options);

	/* static threads overwrite it afterwards with real value */
	thread->init_data = NULL;
	thread->fn_abort = NULL;

#ifdef CONFIG_THREAD_CUSTOM_DATA
	/* Initialize custom data field (value is opaque to kernel) */

	thread->custom_data = NULL;
#endif

	/*
	 * The creation of the initial stack for the task has already been done.
	 * Now all that is needed is to set the ESP. However, we have been passed
	 * the base address of the stack which is past the initial stack frame.
	 * Therefore some of the calculations done in the other routines that
	 * initialize the stack frame need to be repeated.
	 */

	pInitialCtx = (unsigned long *)STACK_ROUND_DOWN(pStackMem + stackSize);

#ifdef CONFIG_THREAD_MONITOR
	/*
	 * In debug mode thread->entry give direct access to the thread entry
	 * and the corresponding parameters.
	 */
	thread->entry = (struct __thread_entry *)(pInitialCtx -
		sizeof(struct __thread_entry));
#endif

	/* The stack needs to be set up so that when we do an initial switch
	 * to it in the middle of _Swap(), it needs to be set up as follows:
	 *  - 4 thread entry routine parameters
	 *  - eflags
	 *  - eip (so that _Swap() "returns" to the entry point)
	 *  - edi, esi, ebx, ebp,  eax
	 */
	pInitialCtx -= 11;

	thread->callee_saved.esp = (unsigned long)pInitialCtx;
	PRINTK("\nInitial context ESP = 0x%x\n", thread->coopReg.esp);

	PRINTK("\nstruct thread * = 0x%x", thread);

	thread_monitor_init(thread);
}

#if defined(CONFIG_GDB_INFO) || defined(CONFIG_DEBUG_INFO) \
	|| defined(CONFIG_X86_IAMCU)
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

/**
 *
 * @brief Create a new kernel execution thread
 *
 * This function is utilized to create execution threads for both fiber
 * threads and kernel tasks.
 *
 * The k_thread structure is carved from the "end" of the specified
 * thread stack memory.
 *
 * @param pStackmem the pointer to aligned stack memory
 * @param stackSize the stack size in bytes
 * @param pEntry thread entry point routine
 * @param parameter1 first param to entry point
 * @param parameter2 second param to entry point
 * @param parameter3 third param to entry point
 * @param priority thread priority
 * @param options thread options: K_ESSENTIAL, K_FP_REGS, K_SSE_REGS
 *
 *
 * @return opaque pointer to initialized k_thread structure
 */
void _new_thread(char *pStackMem, size_t stackSize,
		 _thread_entry_t pEntry,
		 void *parameter1, void *parameter2, void *parameter3,
		 int priority, unsigned options)
{
	_ASSERT_VALID_PRIO(priority, pEntry);

	unsigned long *pInitialThread;

#ifdef CONFIG_INIT_STACKS
	memset(pStackMem, 0xaa, stackSize);
#endif

	/* carve the thread entry struct from the "base" of the stack */

	pInitialThread =
		(unsigned long *)STACK_ROUND_DOWN(pStackMem + stackSize);

	/*
	 * Create an initial context on the stack expected by the _Swap()
	 * primitive.
	 */

	/* push arguments required by _thread_entry() */

	*--pInitialThread = (unsigned long)parameter3;
	*--pInitialThread = (unsigned long)parameter2;
	*--pInitialThread = (unsigned long)parameter1;
	*--pInitialThread = (unsigned long)pEntry;

	/* push initial EFLAGS; only modify IF and IOPL bits */

	*--pInitialThread = (EflagsGet() & ~EFLAGS_MASK) | EFLAGS_INITIAL;

#if defined(CONFIG_GDB_INFO) || defined(CONFIG_DEBUG_INFO) \
	|| defined(CONFIG_X86_IAMCU)
	/*
	 * Arrange for the _thread_entry_wrapper() function to be called
	 * to adjust the stack before _thread_entry() is invoked.
	 */

	*--pInitialThread = (unsigned long)_thread_entry_wrapper;

#else /* defined(CONFIG_GDB_INFO) || defined(CONFIG_DEBUG_INFO) */

	*--pInitialThread = (unsigned long)_thread_entry;

#endif /* defined(CONFIG_GDB_INFO) || defined(CONFIG_DEBUG_INFO) */

	/*
	 * note: stack area for edi, esi, ebx, ebp, and eax registers can be
	 * left
	 * uninitialized, since _thread_entry() doesn't care about the values
	 * of these registers when it begins execution
	 */

	/*
	 * The k_thread structure is located at the "low end" of memory set
	 * aside for the thread's stack.
	 */

	_new_thread_internal(pStackMem, stackSize, priority, options);
}
