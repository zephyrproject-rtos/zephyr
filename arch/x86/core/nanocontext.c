/* nanocontext.c - VxMicro nanokernel context support primitives */

/*
 * Copyright (c) 2010-2015 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This module provides core nanokernel fiber related primitives for the IA-32
processor architecture.
*/

/* includes */

#ifdef CONFIG_MICROKERNEL
#include <microkernel.h>
#include <kernel_struct.h>
#endif /* CONFIG_MICROKERNEL */

#include <toolchain.h>
#include <sections.h>
#include <nanok.h>
#include <nanocontextentry.h>


/* the one and only nanokernel control structure */

tNANO _nanokernel = {0};

/* forward declaration */

#ifdef CONFIG_GDB_INFO
void _ContextEntryWrapper(_ContextEntry, _ContextArg, _ContextArg, _ContextArg);
#endif /* CONFIG_GDB_INFO */

/*******************************************************************************
*
* _NewContextInternal - initialize a new execution context
*
* This function is utilized to initialize all execution contexts, both fiber
* contexts, kernel task contexts and user mode task contexts.  The 'priority'
* parameter will be set to -1 for the creation of task context.
*
* This function is called by _NewContext() to initialize task contexts.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static void _NewContextInternal(
	tCCS *ccs,	  /* pointer to the new task's ccs */
	char *pStackMem,    /* pointer to context stack memory */
	unsigned stackSize, /* size of stack in bytes */
	int priority,       /* context priority */
	unsigned options    /* context options: USE_FP, USE_SSE */
	)
{
	unsigned long *pInitialCtx;

#ifndef CONFIG_FP_SHARING
	ARG_UNUSED(options);
#endif /* !CONFIG_FP_SHARING */

	ccs->link = (tCCS *)NULL; /* context not inserted into list yet */
	ccs->prio = priority;
#if (defined(CONFIG_FP_SHARING) || defined(CONFIG_GDB_INFO))
	ccs->excNestCount = 0;
#endif /* CONFIG_FP_SHARING || CONFIG_GDB_INFO */


	if (priority == -1)
		ccs->flags = PREEMPTIBLE | TASK;
	else
		ccs->flags = FIBER;

#ifdef CONFIG_CONTEXT_CUSTOM_DATA
	/* Initialize custom data field (value is opaque to kernel) */

	ccs->custom_data = NULL;
#endif


	/*
	 * The creation of the initial stack for the task (user or kernel) has
	 * already been done. Now all that is needed is to set the ESP. However,
	 * we have been passed the base address of the stack which is past the
	 * initial stack frame. Therefore some of the calculations done in the
	 * other routines that initialize the stack frame need to be repeated.
	 */

	pInitialCtx = (unsigned long *)STACK_ROUND_DOWN(pStackMem + stackSize);

	/*
	 * We subtract 11 here to account for the context entry routine
	 * parameters
	 * (4 of them), eflags, eip, and the edi/esi/ebx/ebp/eax registers.
	 */
	pInitialCtx -= 11;

	ccs->coopReg.esp = (unsigned long)pInitialCtx;
	PRINTK("\nInitial context ESP = 0x%x\n", ccs->coopReg.esp);

#ifdef CONFIG_FP_SHARING
/*
 * Indicate if the context is permitted to use floating point instructions.
 *
 * The first time the new context is scheduled by _Swap() it is guaranteed
 * to inherit an FPU that is in a "sane" state (if the most recent user of
 * the FPU was cooperatively swapped out) or a completely "clean" state
 * (if the most recent user of the FPU was pre-empted, or if the new context
 * is the first user of the FPU).
 *
 * The USE_FP flag bit is set in the tCCS structure if a context is
 * authorized to use _any_ non-integer capability, whether it's the basic
 * x87 FPU/MMX capability, SSE instructions, or a combination of both. The
 * USE_SSE flag bit is set only if a context can use SSE instructions.
 *
 * Note: Callers need not follow the aforementioned protocol when passing
 * in context options. It is legal for the caller to specify _only_ the
 * USE_SSE option bit if a context will be utilizing SSE instructions (and
 * possibly x87 FPU/MMX instructions).
 */

/*
 * Implementation Remark:
 * Until SysGen reserves SSE_GROUP as 0x10, the following conditional is
 * required so that at least systems configured with FLOAT will still operate
 * correctly.  The issue is that SysGen will utilize group 0x10 user-defined
 * groups, and thus tasks placed in the user-defined group will have the
 * SSE_GROUP (but not the FPU_GROUP) bit set.  This results in both the USE_FP
 * and USE_SSE bits being set in the tCCS.  For systems configured only with
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
		ccs->flags |= (options | USE_FP);
	}
#endif /* CONFIG_FP_SHARING */

	PRINTK("\ntCCS * = 0x%x", ccs);

#if defined(CONFIG_CONTEXT_MONITOR)
	{
		unsigned int imask;

		/*
		 * Add the newly initialized context to head of the list of
		 * contexts.
		 * This singly linked list of contexts maintains ALL the
		 * contexts in the
		 * system: both tasks and fibers regardless of whether they are
		 * runnable.
		 */

		imask = irq_lock();
		ccs->next_context = _nanokernel.contexts;
		_nanokernel.contexts = ccs;
		irq_unlock(imask);
	}
#endif /* CONFIG_CONTEXT_MONITOR */
}

#ifdef CONFIG_GDB_INFO
/*******************************************************************************
*
* _ContextEntryWrapper - adjust stack before invoking _context_entry
*
* This function adjusts the initial stack frame created by _NewContext()
* such that the GDB stack frame unwinders recognize it as the outermost frame
* in the context's stack.  The function then jumps to _context_entry().
*
* GDB normally stops unwinding a stack when it detects that it has
* reached a function called main().  Kernel tasks, however, do not have
* a main() function, and there does not appear to be a simple way of stopping
* the unwinding of the stack.
*
* Given the initial context created by _NewContext(), GDB expects to find a
* return address on the stack immediately above the context entry routine
* _context_entry, in the location occupied by the initial EFLAGS.
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
*      |    entryRtn      |  <-----  Context Entry Routine invoked by _Swap()
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
* the new context for the first time.  This routine is called by _Swap() the
* first time that the new context is swapped in, and it jumps to
* _context_entry after it has done its work.
*
* RETURNS: this routine does NOT return.
*
* \NOMANUAL
*/

__asm__("\t.globl _context_entry\n"
	"\t.section .text\n"
	"_ContextEntryWrapper:\n" /* should place this func .s file and use
				     SECTION_FUNC */
	"\tmovl $0, (%esp)\n" /* zero initialEFLAGS location */
	"\tjmp _context_entry\n");
#endif /* CONFIG_GDB_INFO */

/*******************************************************************************
*
* _NewContext - create a new kernel execution context
*
* This function is utilized to create execution contexts for both fiber
* contexts and kernel task contexts.
*
* The "context control block" (CCS) is carved from the "end" of the specified
* context stack memory.
*
* RETURNS: opaque pointer to initialized CCS structure
*
* \NOMANUAL
*/

void *_NewContext(
	char *pStackMem,      /* pointer to context stack memory */
	unsigned stackSize,   /* size of stack in bytes */
	_ContextEntry pEntry, /* context entry point function */
	void *parameter1, /* first parameter to context entry point function */
	void *parameter2, /* second parameter to context entry point function */
	void *parameter3, /* third parameter to context entry point function */
	int priority,     /* context priority */
	unsigned options  /* context options: USE_FP, USE_SSE */
	)
{
	tCCS *ccs;
	unsigned long *pInitialContext;

	/* carve the context entry struct from the "base" of the stack */

	pInitialContext =
		(unsigned long *)STACK_ROUND_DOWN(pStackMem + stackSize);

	/*
	 * Create an initial context on the stack expected by the _Swap()
	 * primitive.
	 * Given that both task and fiber contexts execute at privilege 0, the
	 * setup for both contexts are equivalent.
	 */

	/* push arguments required by _context_entry() */

	*--pInitialContext = (unsigned long)parameter3;
	*--pInitialContext = (unsigned long)parameter2;
	*--pInitialContext = (unsigned long)parameter1;
	*--pInitialContext = (unsigned long)pEntry;

	/* push initial EFLAGS; only modify IF and IOPL bits */

	*--pInitialContext = (EflagsGet() & ~EFLAGS_MASK) | EFLAGS_INITIAL;

#ifdef CONFIG_GDB_INFO

	/*
	 * Arrange for the _ContextEntryWrapper() function to be called
	 * to adjust the stack before _context_entry() is invoked.
	 */

	*--pInitialContext = (unsigned long)_ContextEntryWrapper;

#else /* CONFIG_GDB_INFO */

	*--pInitialContext = (unsigned long)_context_entry;

#endif /* CONFIG_GDB_INFO */

	/*
	 * note: stack area for edi, esi, ebx, ebp, and eax registers can be
	 * left
	 * uninitialized, since _context_entry() doesn't care about the values
	 * of these registers when it begins execution
	 */

	/*
	 * For kernel tasks and fibers the context the context control struct
	 * (CCS)
	 * is located at the "low end" of memory set aside for the context's
	 * stack
	 */

	ccs = (tCCS *)ROUND_UP(pStackMem, CCS_ALIGN);

	_NewContextInternal(ccs, pStackMem, stackSize, priority, options);


	return ((void *)ccs);
}
