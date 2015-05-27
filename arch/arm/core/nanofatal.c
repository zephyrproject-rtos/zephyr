/* nanofatal.c - nanokernel fatal error handler for ARM Cortex-M */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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
This module provides the _NanoFatalErrorHandler() routine for ARM Cortex-M.
*/

/* includes */

#include <toolchain.h>
#include <sections.h>

#include <cputype.h>
#include <nanokernel.h>
#include <arch/cpu.h>
#include <nanok.h>

#ifdef CONFIG_PRINTK
#include <misc/printk.h>
#define PR_EXC(...) printk(__VA_ARGS__)
#else
#define PR_EXC(...)
#endif /* CONFIG_PRINTK */

/* globals */

/*
 * Define a default ESF for use with _NanoFatalErrorHandler() in the event
 * the caller does not have a NANO_ESF to pass
 */
const NANO_ESF _default_esf = {0xdeaddead, /* a1 */
						      0xdeaddead, /* a2 */
						      0xdeaddead, /* a3 */
						      0xdeaddead, /* a4 */
						      0xdeaddead, /* ip */
						      0xdeaddead, /* lr */
						      0xdeaddead, /* pc */
						      0xdeaddead, /* xpsr */
};

/*******************************************************************************
*
* _NanoFatalErrorHandler - nanokernel fatal error handler
*
* This routine is called when fatal error conditions are detected by software
* and is responsible only for reporting the error. Once reported, it then
* invokes the user provided routine _SysFatalErrorHandler() which is
* responsible for implementing the error handling policy.
*
* The caller is expected to always provide a usable ESF. In the event that the
* fatal error does not have a hardware generated ESF, the caller should either
* create its own or use a pointer to the global default ESF <_default_esf>.
*
* RETURNS: This function does not return.
*
* \NOMANUAL
*/

FUNC_NORETURN void _NanoFatalErrorHandler(
	unsigned int reason, /* reason that handler was called */
	const NANO_ESF *pEsf /* pointer to exception stack frame */
	)
{
	switch (reason) {
	case _NANO_ERR_INVALID_TASK_EXIT:
		PR_EXC("***** Invalid Exit Software Error! *****\n");
		break;

#if defined(CONFIG_STACK_CANARIES)
	case _NANO_ERR_STACK_CHK_FAIL:
		PR_EXC("***** Stack Check Fail! *****\n");
		break;
#endif /* CONFIG_STACK_CANARIES */

	default:
		PR_EXC("**** Unknown Fatal Error %d! ****\n", reason);
		break;
	}
	PR_EXC("Current context ID = 0x%x\n"
	       "Faulting instruction address = 0x%x\n",
	       context_self_get(),
	       pEsf->pc);

	/*
	 * Now that the error has been reported, call the user implemented
	 * policy
	 * to respond to the error.  The decisions as to what responses are
	 * appropriate to the various errors are something the customer must
	 * decide.
	 */

	_SysFatalErrorHandler(reason, pEsf);

	for (;;)
		;
}
