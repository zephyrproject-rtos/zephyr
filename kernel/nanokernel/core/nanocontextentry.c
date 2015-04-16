/* nanocontextentry.c - VxMicro nanokernel context entry/exit logic */

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
This module provides a wrapper around a context entry point for starting and
ending the context, be it a task or a fiber.
*/

#include <toolchain.h>
#include <sections.h>

#ifdef CONFIG_MICROKERNEL
#include <microkernel/k_struct.h>
#include <microkernel.h>
#endif /* CONFIG_MICROKERNEL */

#include <nanok.h>
#include <nanocontextentry.h>
#include <misc/printk.h>

#if defined(CONFIG_HOST_TOOLS_SUPPORT)
/*******************************************************************************
*
* _context_exit - context exit routine
*
* This function is invoked when the specified context is aborted, either
* normally or abnormally. It is called for the termination of any context,
* (fibers and tasks).
*
* This routine must be invoked from a fiber to guarantee that the list
* of contexts does not change in mid-operation.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _context_exit(tCCS *pContext)
{
	/*
	 * Remove context from the list of contexts.  This singly linked list of
	 * contexts maintains ALL the contexts in the system: both tasks and
	 * fibers regardless of whether they are runnable.
	 */

	if (pContext == _NanoKernel.contexts) {
		_NanoKernel.contexts = _NanoKernel.contexts->activeLink;
	} else {
		tCCS *pPrevContext;

		pPrevContext = _NanoKernel.contexts;
		while (pContext != pPrevContext->activeLink) {
			pPrevContext = pPrevContext->activeLink;
		}
		pPrevContext->activeLink = pContext->activeLink;
	}
}
#endif /* CONFIG_HOST_TOOLS_SUPPORT */

/*******************************************************************************
*
* _context_entry - common context entry point function for kernel contexts
*
* This function serves as the entry point for _all_ kernel contexts, i.e. both
* task and fiber contexts are instantiated such that initial execution starts
* here.
*
* This routine invokes the actual task or fiber entry point function and
* passes it three arguments.  It also handles graceful termination of the
* task or fiber if the entry point function ever returns.
*
* INTERNAL
* The 'noreturn' attribute is applied to this function so that the compiler
* can dispense with generating the usual preamble that is only required for
* functions that actually return.
*
* The analogous entry point function for user-mode task contexts is called
* _ContextUsrEntryRtn().
*
* RETURNS: Does not return
*
* \NOMANUAL
*/

FUNC_NORETURN void _context_entry(
	_ContextEntry pEntry,   /* address of app entry point function */
	_ContextArg parameter1, /* 1st arg to app entry point function */
	_ContextArg parameter2, /* 2nd arg to app entry point function */
	_ContextArg parameter3  /* 3rd arg to app entry point function */
	)
{
	/* Execute the "application" entry point function */

	pEntry(parameter1, parameter2, parameter3);

	/* Determine if context can legally terminate itself via "return" */

	if (_context_essential_check(NULL)) {
#ifdef CONFIG_NANOKERNEL
		/*
		 * Nanokernel's background task must always be present,
		 * so if it has nothing left to do just let it idle forever
		 */

		while (((_NanoKernel.current)->flags & TASK) == TASK) {
			nano_cpu_idle();
		}
#endif /*  CONFIG_NANOKERNEL */

		/* Loss of essential context is a system fatal error */

		_NanoFatalErrorHandler(_NANO_ERR_INVALID_TASK_EXIT,
				       &__defaultEsf);
	}

/* Gracefully terminate the currently executing context */

#ifdef CONFIG_MICROKERNEL
	if (((_NanoKernel.current)->flags & TASK) == TASK) {
		extern FUNC_NORETURN void _TaskAbort(void);
		_TaskAbort();
	} else
#endif /* CONFIG_MICROKERNEL */
	{
		fiber_abort();
	}

	/*
	 * Compiler can't tell that fiber_abort() won't return and issues
	 * a warning unless we explicitly tell it that control never gets this
	 * far.
	 */

	CODE_UNREACHABLE;
}
