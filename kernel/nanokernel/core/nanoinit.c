/* nanoinit.c - VxMicro nanokernel initialization module */

/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
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
This module contains routines that are used to initialize the nanokernel.
*/

#include <toolchain.h>
#include <sections.h>

#ifdef CONFIG_MICROKERNEL
#include <minik.h>
#endif /* CONFIG_MICROKERNEL */

#include <nanok.h>

/* stack space for the background task context */

static char __noinit mainStack[CONFIG_MAIN_STACK_SIZE];

/* storage space for the interrupt stack */

#ifndef CONFIG_NO_ISRS

/*
 * The symbol for the interrupt stack area is NOT static since it's
 * referenced from a BSPs crt0.s module when setting up the temporary
 * stack used during the execution of kernel initialization sequence, i.e.
 * up until the first context switch.  The dual-purposing of this area of
 * memory is safe since interrupts are disabled until the first context
 * switch.
 *
 * The NO_ISRS option is only supported with the 'lxPentium4' BSP, but
 * this BSP doesn't need to setup the aforementioned temporary stack.
 */

char __noinit _interrupt_stack[CONFIG_ISR_STACK_SIZE];
#endif

/*
 * The kernel allows a BSP to pass arguments to the background task in
 * a nanokernel-only system, or the idle task in a microkernel system,
 * but this isn't advertised at the moment since there isn't a use case
 * for this capability. However, one may arise in a future release ...
 */

extern void main(int argc, char *argv[], char *envp[]);

/*******************************************************************************
*
* _nano_init - Initializes the nanokernel layer
*
* This function is invoked from a BSP's initialization routine, which is in
* turn invoked by crt0.s.  The following is a summary of the early nanokernel
* initialization sequence:
*
*   crt0.s  -> _Cstart() -> _nano_init()
*                        -> _nano_fiber_swap()
*
*   main () -> kernel_init () -> task_fiber_start(... K_swapper ...)
*
* The _nano_init() routine initializes a context for the main() routine
* (aka background context which is a task context)), and sets _NanoKernel.task
* to the 'tCCS *' for the new context.  The _NanoKernel.current field is set to
* the provided <dummyOutContext> tCCS, however _NanoKernel.fiber is set to
* NULL.
*
* Thus the subsequent invocation of _nano_fiber_swap() depicted above results
* in a context switch into the main() routine.  The <dummyOutContext> will
* be used as the context save area for the swap.  Typically, <dummyOutContext>
* is a temp area on the current stack (as setup by crt0.s).
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _nano_init(tCCS *dummyOutContext, int argc, char *argv[], char *envp[])
{
	/*
	 * Setup enough information re: the current execution context to permit
	 * a level of debugging output if an exception should happen to occur
	 * during _IntLibInit() or _AppContextInit(), for example. However,
	 * don't
	 * waste effort initializing the fields of the dummy context beyond
	 * those
	 * needed to identify it as a dummy context.
	 */

	_NanoKernel.current = dummyOutContext;

	dummyOutContext->link =
		(tCCS *)NULL; /* context not inserted into list */
	dummyOutContext->flags = FIBER | ESSENTIAL;
	dummyOutContext->prio = 0;


#ifndef CONFIG_NO_ISRS
	/*
	 * The interrupt library needs to be initialized early since a series of
	 * handlers are installed into the interrupt table to catch spurious
	 * interrupts.  This must be performed before other nanokernel
	 * subsystems
	 * install bonafide handlers, or before hardware device drivers are
	 * initialized (in the BSPs' _InitHardware).
	 */

	_IntLibInit();
#endif

#if defined(CONFIG_MICROKERNEL)
#endif

#if defined(CONFIG_HOST_TOOLS_SUPPORT)
/* initialize the linked list of all contexts */
/* _NanoKernel.contexts  = NULL; not needed since _NanoKernel is in .bss */
#endif /* CONFIG_HOST_TOOLS_SUPPORT */

	/*
	 * Initialize the context control block (CCS) for the main (aka
	 * background)
	 * context.  The entry point for the background context is hardcoded as
	 * 'main'.
	 */

	_NanoKernel.task =
		_NewContext(mainStack,			 /* pStackMem */
			    CONFIG_MAIN_STACK_SIZE, /* stackSize */
			    (_ContextEntry)main,	 /* pEntry */
			    (_ContextArg)argc,		 /* parameter1 */
			    (_ContextArg)argv,		 /* parameter2 */
			    (_ContextArg)envp,		 /* parameter3 */
			    -1,				 /* priority */
			    0				 /* options */
			    );

	/* indicate that failure of this task may be fatal to the entire system
	 */

	_NanoKernel.task->flags |= ESSENTIAL;

#if defined(CONFIG_MICROKERNEL)
	/* fill in microkernel's TCB, which is the last element in _k_task_list[]
	 */

	_k_task_list[K_TaskCount].workspace = (char *)_NanoKernel.task;
	_k_task_list[K_TaskCount].worksize = CONFIG_MAIN_STACK_SIZE;
#endif

	/*
	 * Initialize the nanokernel (tNANO) structure specifying the dummy
	 * context
	 * as the currently executing fiber context.
	 */

	_NanoKernel.fiber = NULL;
#ifdef CONFIG_FP_SHARING
	_NanoKernel.current_fp = NULL;
#endif /* CONFIG_FP_SHARING */

	nanoArchInit();
}
