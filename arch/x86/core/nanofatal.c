/* nanofatal.c - nanokernel fatal error handler */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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
This module provides the _NanoFatalErrorHandler() routine.
*/

#include <toolchain.h>
#include <sections.h>

#include <nanokernel.h>
#include <nanok.h>
#include <misc/printk.h>


/*
 * Define a default ESF for use with _NanoFatalErrorHandler() in the event
 * the caller does not have a NANO_ESF to pass
 */
const NANO_ESF _default_esf = {
#ifdef CONFIG_GDB_INFO
	0xdeaddead, /* EBP */
	0xdeaddead, /* EBX */
	0xdeaddead, /* ESI */
	0xdeaddead, /* EDI */
#endif		    /* CONFIG_GDB_INFO */
	0xdeaddead, /* EDX */
	0xdeaddead, /* ECX */
	0xdeaddead, /* EAX */
	0xdeaddead, /* error code */
	0xdeaddead, /* EIP */
	0xdeaddead, /* CS */
	0xdeaddead, /* EFLAGS */
	0xdeaddead, /* ESP */
	0xdeaddead  /* SS */
};

/*******************************************************************************
*
* _NanoFatalErrorHandler - nanokernel fatal error handler
*
* This routine is called when a fatal error condition is detected by either
* hardware or software.
*
* The caller is expected to always provide a usable ESF.  In the event that the
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

#ifdef CONFIG_PRINTK

	/* Display diagnostic information about the error */

	switch (reason) {
	case _NANO_ERR_SPURIOUS_INT:
		printk("***** Unhandled exception/interrupt occurred! "
		       "*****\n");
		break;


	case _NANO_ERR_INVALID_TASK_EXIT:
		printk("***** Invalid Exit Software Error! *****\n");
		break;

#if defined(CONFIG_STACK_CANARIES)
	case _NANO_ERR_STACK_CHK_FAIL:
		printk("***** Stack Check Fail! *****\n");
		break;
#endif /* CONFIG_STACK_CANARIES */


	case _NANO_ERR_ALLOCATION_FAIL:
		printk("**** Kernel Allocation Failure! ****\n");
		break;

	default:
		printk("**** Unknown Fatal Error %d! ****\n", reason);
		break;
	}

	printk("Current context ID = 0x%x\n"
	       "Faulting instruction address = 0x%x\n",
	       context_self_get(),
	       pEsf->eip);
#endif /* CONFIG_PRINTK */


	/*
	 * Error was fatal to a kernel task or a fiber,
	 * so invoke the system fatal error handling policy defined for the BSP
	 */

	_SysFatalErrorHandler(reason, pEsf);
}
