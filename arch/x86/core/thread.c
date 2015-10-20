/* thread.c - nanokernel thread support primitives */

/*
 * Copyright (c) 2010-2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * DESCRIPTION
 * This module provides core nanokernel fiber related primitives for the IA-32
 * processor architecture.
 */

#ifdef CONFIG_MICROKERNEL
#include <microkernel.h>
#include <micro_private_types.h>
#endif /* CONFIG_MICROKERNEL */
#ifdef CONFIG_INIT_STACKS
#include <string.h>
#endif /* CONFIG_INIT_STACKS */

#include <toolchain.h>
#include <sections.h>
#include <nano_private.h>
#include <wait_q.h>

/* the one and only nanokernel control structure */

tNANO _nanokernel = {0};

/* forward declaration */

#ifdef CONFIG_GDB_INFO
void _thread_entry_wrapper(_thread_entry_t, _thread_arg_t,
			   _thread_arg_t, _thread_arg_t);
#endif /* CONFIG_GDB_INFO */

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
 * @param thread priority
 * @param options thread options: USE_FP, USE_SSE
 *
 * @return N/A
 *
 * \NOMANUAL
 */

static void _new_thread_internal(char *pStackMem, unsigned stackSize,
				 int priority, unsigned options)
{
	unsigned long *pInitialCtx;
	/* ptr to the new task's tcs */
	struct tcs *tcs = (struct tcs *)pStackMem;

#ifndef CONFIG_FP_SHARING
	ARG_UNUSED(options);
#endif /* !CONFIG_FP_SHARING */

	tcs->link = (struct tcs *)NULL; /* thread not inserted into list yet */
	tcs->prio = priority;
#if (defined(CONFIG_FP_SHARING) || defined(CONFIG_GDB_INFO))
	tcs->excNestCount = 0;
#endif /* CONFIG_FP_SHARING || CONFIG_GDB_INFO */


	if (priority == -1)
		tcs->flags = PREEMPTIBLE | TASK;
	else
		tcs->flags = FIBER;

#ifdef CONFIG_THREAD_CUSTOM_DATA
	/* Initialize custom data field (value is opaque to kernel) */

	tcs->custom_data = NULL;
#endif


	/*
	 * The creation of the initial stack for the task has already been done.
	 * Now all that is needed is to set the ESP. However, we have been passed
	 * the base address of the stack which is past the initial stack frame.
	 * Therefore some of the calculations done in the other routines that
	 * initialize the stack frame need to be repeated.
	 */

	pInitialCtx = (unsigned long *)STACK_ROUND_DOWN(pStackMem + stackSize);

	/*
	 * We subtract 11 here to account for the thread entry routine
	 * parameters
	 * (4 of them), eflags, eip, and the edi/esi/ebx/ebp/eax registers.
	 */
	pInitialCtx -= 11;

	tcs->coopReg.esp = (unsigned long)pInitialCtx;
	PRINTK("\nInitial context ESP = 0x%x\n", tcs->coopReg.esp);

#ifdef CONFIG_FP_SHARING
/*
 * Indicate if the thread is permitted to use floating point instructions.
 *
 * The first time the new thread is scheduled by _Swap() it is guaranteed
 * to inherit an FPU that is in a "sane" state (if the most recent user of
 * the FPU was cooperatively swapped out) or a completely "clean" state
 * (if the most recent user of the FPU was pre-empted, or if the new thread
 * is the first user of the FPU).
 *
 * The USE_FP flag bit is set in the struct tcs structure if a thread is
 * authorized to use _any_ non-integer capability, whether it's the basic
 * x87 FPU/MMX capability, SSE instructions, or a combination of both. The
 * USE_SSE flag bit is set only if a thread can use SSE instructions.
 *
 * Note: Callers need not follow the aforementioned protocol when passing
 * in thread options. It is legal for the caller to specify _only_ the
 * USE_SSE option bit if a thread will be utilizing SSE instructions (and
 * possibly x87 FPU/MMX instructions).
 */

/*
 * Implementation Remark:
 * Until SysGen reserves SSE_GROUP as 0x10, the following conditional is
 * required so that at least systems configured with FLOAT will still operate
 * correctly.  The issue is that SysGen will utilize group 0x10 user-defined
 * groups, and thus tasks placed in the user-defined group will have the
 * SSE_GROUP (but not the FPU_GROUP) bit set.  This results in both the USE_FP
 * and USE_SSE bits being set in the struct tcs.  For systems configured only with
 * FLOAT, the setting of the USE_SSE is harmless, but the setting of USE_FP is
 * wasteful.  Thus to ensure that that systems configured only with FLOAT
 * behave as expected, the USE_SSE option bit is ignored.
 *
 * Clearly, even with the following conditional, systems configured with
 * SSE will not behave as expected since tasks may still be inadvertantly
 * have the USE_SSE+USE_FP sets even though they are integer only.
 *
 * Once the generator tool has been updated to reserve the SSE_GROUP, the
 * correct code to use is:
 *
 *    options &= USE_FP | USE_SSE;
 *
 */

#ifdef CONFIG_SSE
	options &= USE_FP | USE_SSE;
#else
	options &= USE_FP;
#endif

	if (options != 0) {
		tcs->flags |= (options | USE_FP);
	}
#endif /* CONFIG_FP_SHARING */

	PRINTK("\nstruct tcs * = 0x%x", tcs);

#if defined(CONFIG_THREAD_MONITOR)
	{
		unsigned int imask;

		/*
		 * Add the newly initialized thread to head of the list of threads.
		 * This singly linked list of threads maintains ALL the threads in the
		 * system: both tasks and fibers regardless of whether they are
		 * runnable.
		 */

		imask = irq_lock();
		tcs->next_thread = _nanokernel.threads;
		_nanokernel.threads = tcs;
		irq_unlock(imask);
	}
#endif /* CONFIG_THREAD_MONITOR */

	_nano_timeout_tcs_init(tcs);
}

#ifdef CONFIG_GDB_INFO
/**
 *
 * @brief Adjust stack before invoking _thread_entry
 *
 * This function adjusts the initial stack frame created by _new_thread()
 * such that the GDB stack frame unwinders recognize it as the outermost frame
 * in the thread's stack.  The function then jumps to _thread_entry().
 *
 * GDB normally stops unwinding a stack when it detects that it has
 * reached a function called main().  Kernel tasks, however, do not have
 * a main() function, and there does not appear to be a simple way of stopping
 * the unwinding of the stack.
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
 *       __________________
 *      |      param3      |   <------ Top of the stack
 *      |__________________|
 *      |      param2      |           Stack Grows Down
 *      |__________________|                  |
 *      |      param1      |                  V
 *      |__________________|
 *      |      pEntry      |
 *      |__________________|
 *      | initial EFLAGS   |  <----   ESP when invoked by _Swap()
 *      |__________________|             (Zeroed by this routine)
 *      |    entryRtn      |  <-----  Thread Entry Routine invoked by _Swap()
 *      |__________________|             (This routine if GDB_INFO)
 *      |      <edi>       |  \
 *      |__________________|  |
 *      |      <esi>       |  |
 *      |__________________|  |
 *      |      <ebx>       |  |----   Initial registers restored by _Swap()
 *      |__________________|  |
 *      |      <ebp>       |  |
 *      |__________________|  |
 *      |      <eax>       | /
 *      |__________________|
 *
 *
 * The initial EFLAGS cannot be overwritten until after _Swap() has swapped in
 * the new thread for the first time.  This routine is called by _Swap() the
 * first time that the new thread is swapped in, and it jumps to
 * _thread_entry after it has done its work.
 *
 * @return this routine does NOT return.
 *
 * \NOMANUAL
 */

__asm__("\t.globl _thread_entry\n"
	"\t.section .text\n"
	"_thread_entry_wrapper:\n" /* should place this func .S file and use
				    * SECTION_FUNC
				    */
	"\tmovl $0, (%esp)\n" /* zero initialEFLAGS location */
	"\tjmp _thread_entry\n");
#endif /* CONFIG_GDB_INFO */

/**
 *
 * @brief Create a new kernel execution thread
 *
 * This function is utilized to create execution threads for both fiber
 * threads and kernel tasks.
 *
 * The "thread control block" (TCS) is carved from the "end" of the specified
 * thread stack memory.
 *
 * @param pStackmem the pointer to aligned stack memory
 * @param stackSize the stack size in bytes
 * @param pEntry thread entry point routine
 * @param parameter1 first param to entry point
 * @param parameter2 second param to entry point
 * @param parameter3 third param to entry point
 * @param priority  thread priority
 * @param options thread options: USE_FP, USE_SSE
 *
 *
 * @return opaque pointer to initialized TCS structure
 *
 * \NOMANUAL
 */

void _new_thread(char *pStackMem, unsigned stackSize, _thread_entry_t pEntry,
		 void *parameter1, void *parameter2, void *parameter3,
		 int priority, unsigned options)
{
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
	 * Given that both task and fibers execute at privilege 0, the
	 * setup for both threads are equivalent.
	 */

	/* push arguments required by _thread_entry() */

	*--pInitialThread = (unsigned long)parameter3;
	*--pInitialThread = (unsigned long)parameter2;
	*--pInitialThread = (unsigned long)parameter1;
	*--pInitialThread = (unsigned long)pEntry;

	/* push initial EFLAGS; only modify IF and IOPL bits */

	*--pInitialThread = (EflagsGet() & ~EFLAGS_MASK) | EFLAGS_INITIAL;

#ifdef CONFIG_GDB_INFO

	/*
	 * Arrange for the _thread_entry_wrapper() function to be called
	 * to adjust the stack before _thread_entry() is invoked.
	 */

	*--pInitialThread = (unsigned long)_thread_entry_wrapper;

#else /* CONFIG_GDB_INFO */

	*--pInitialThread = (unsigned long)_thread_entry;

#endif /* CONFIG_GDB_INFO */

	/*
	 * note: stack area for edi, esi, ebx, ebp, and eax registers can be
	 * left
	 * uninitialized, since _thread_entry() doesn't care about the values
	 * of these registers when it begins execution
	 */

	/*
	 * For kernel tasks and fibers the thread the thread control struct (TCS)
	 * is located at the "low end" of memory set aside for the thread's stack.
	 */

	_new_thread_internal(pStackMem, stackSize, priority, options);
}
