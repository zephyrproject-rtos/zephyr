/* nanokernel context support */

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
This module provides general purpose context support, with applies to both
tasks or fibers.
 */

#include <toolchain.h>
#include <sections.h>

#include <nano_private.h>
#include <misc/printk.h>

/**
 *
 * @brief Return the currently executing context
 *
 * This routine returns a pointer to the context control block of the currently
 * executing context.  It is cast to a nano_context_id_t for use publically.
 *
 * @return nano_context_id_t of the currently executing context.
 */

nano_context_id_t context_self_get(void)
{
	return _nanokernel.current;
}

/**
 *
 * @brief Return the type of the currently executing context
 *
 * This routine returns the type of context currently executing.
 *
 * @return nano_context_type_t of the currently executing context.
 */

nano_context_type_t context_type_get(void)
{
	if (_IS_IN_ISR())
		return NANO_CTX_ISR;

	if ((_nanokernel.current->flags & TASK) == TASK)
		return NANO_CTX_TASK;

	return NANO_CTX_FIBER;
}

/**
 *
 * @brief Mark context as essential to system
 *
 * This function tags the running fiber or task as essential to system
 * option; exceptions raised by this context will be treated as a fatal
 * system error.
 *
 * @return N/A
 */

void _context_essential_set(void)
{
	_nanokernel.current->flags |= ESSENTIAL;
}

/**
 *
 * @brief Mark context as not essential to system
 *
 * This function tags the running fiber or task as not essential to system
 * option; exceptions raised by this context may be recoverable.
 * (This is the default tag for a context.)
 *
 * @return N/A
 */

void _context_essential_clear(void)
{
	_nanokernel.current->flags &= ~ESSENTIAL;
}

/**
 *
 * @brief Is the specified context essential?
 *
 * This routine indicates if the specified context is an essential system
 * context.  A NULL context pointer indicates that the current context is
 * to be queried.
 *
 * @return Non-zero if specified context is essential, zero if it is not
 */

int _context_essential_check(tCCS *pCtx /* pointer to context */
					   )
{
	return ((pCtx == NULL) ? _nanokernel.current : pCtx)->flags & ESSENTIAL;
}

#ifdef CONFIG_CONTEXT_CUSTOM_DATA

/**
 *
 * @brief Set context's custom data
 *
 * This routine sets the custom data value for the current task or fiber.
 * Custom data is not used by the kernel itself, and is freely available
 * for the context to use as it sees fit.
 *
 * @return N/A
 */

void context_custom_data_set(void *value /* new value */
		      )
{
	_nanokernel.current->custom_data = value;
}

/**
 *
 * @brief Get context's custom data
 *
 * This function returns the custom data value for the current task or fiber.
 *
 * @return current handle value
 */

void *context_custom_data_get(void)
{
	return _nanokernel.current->custom_data;
}

#endif /* CONFIG_CONTEXT_CUSTOM_DATA */

#if defined(CONFIG_CONTEXT_MONITOR)
/**
 *
 * @brief Context exit routine
 *
 * This function is invoked when the specified context is aborted, either
 * normally or abnormally. It is called for the termination of any context,
 * (fibers and tasks).
 *
 * This routine must be invoked from a fiber to guarantee that the list
 * of contexts does not change in mid-operation.
 *
 * @return N/A
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

	if (pContext == _nanokernel.contexts) {
		_nanokernel.contexts = _nanokernel.contexts->next_context;
	} else {
		tCCS *pPrevContext;

		pPrevContext = _nanokernel.contexts;
		while (pContext != pPrevContext->next_context) {
			pPrevContext = pPrevContext->next_context;
		}
		pPrevContext->next_context = pContext->next_context;
	}
}
#endif /* CONFIG_CONTEXT_MONITOR */

/**
 *
 * @brief Common context entry point function for kernel contexts
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
 * @return Does not return
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

		while (((_nanokernel.current)->flags & TASK) == TASK) {
			nano_cpu_idle();
		}
#endif /*  CONFIG_NANOKERNEL */

		/* Loss of essential context is a system fatal error */

		_NanoFatalErrorHandler(_NANO_ERR_INVALID_TASK_EXIT,
				       &_default_esf);
	}

/* Gracefully terminate the currently executing context */

#ifdef CONFIG_MICROKERNEL
	if (((_nanokernel.current)->flags & TASK) == TASK) {
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
