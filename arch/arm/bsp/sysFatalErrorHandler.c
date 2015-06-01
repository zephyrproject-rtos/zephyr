/* sysFatalErrorHandler - ARM Cortex-M system fatal error handler */

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
This module provides the _SysFatalErrorHandler() routine for Cortex-M BSPs.
*/

#include <cputype.h>
#include <nanokernel.h>
#include <arch/cpu.h>
#include <toolchain.h>
#include <sections.h>
#include "board.h"

#ifdef CONFIG_PRINTK
#include <misc/printk.h>
#define PRINTK(...) printk(__VA_ARGS__)
#else
#define PRINTK(...)
#endif

#ifdef CONFIG_MICROKERNEL
extern void _TaskAbort(void);
static inline void nonEssentialTaskAbort(void)
{
	PRINTK("Fatal fault in task ! Aborting task.\n");
	_TaskAbort();
}
#define NON_ESSENTIAL_TASK_ABORT() nonEssentialTaskAbort()
#else
#define NON_ESSENTIAL_TASK_ABORT() \
	do {/* nothing */          \
	} while ((0))
#endif

/*******************************************************************************
*
* _SysFatalErrorHandler - fatal error handler
*
* This routine implements the corrective action to be taken when the system
* detects a fatal error.
*
* This sample implementation attempts to abort the current context and allow
* the system to continue executing, which may permit the system to continue
* functioning with degraded capabilities.
*
* System designers may wish to enhance or substitute this sample
* implementation to take other actions, such as logging error (or debug)
* information to a persistent repository and/or rebooting the system.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _SysFatalErrorHandler(
	unsigned int reason, /* fatal error reason */
	const NANO_ESF * pEsf /* pointer to exception stack frame */
	)
{
	nano_context_type_t curCtx = context_type_get();

	ARG_UNUSED(reason);
	ARG_UNUSED(pEsf);

	if ((curCtx == NANO_CTX_ISR) || _context_essential_check(NULL)) {
		PRINTK("Fatal fault in %s ! Spinning...\n",
		       NANO_CTX_ISR == curCtx
			       ? "ISR"
			       : NANO_CTX_FIBER == curCtx ? "essential fiber"
							  : "essential task");
		for (;;)
			; /* spin forever */
	}

	if (NANO_CTX_FIBER == curCtx) {
		PRINTK("Fatal fault in fiber ! Aborting fiber.\n");
		fiber_abort();
		return;
	}

	NON_ESSENTIAL_TASK_ABORT();
}
