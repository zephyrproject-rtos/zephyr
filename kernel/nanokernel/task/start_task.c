/* start_task.c - arch-agnostic nanokernel APIs to start a task */

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
This module contains the single primitive 'start_task' which is utilized
by the microkernel to "start a task".
*/

#ifdef CONFIG_MICROKERNEL

#include <start_task_arch.h>

/*******************************************************************************
*
* start_task - initialize and start a task
*
* RETURNS: N/A
*/

void start_task(struct k_proc *X,	 /* ptr to task control block */
			      void (*func)(void) /* entry point for task */
			      )
{
	unsigned int contextOptions;
	void *pNewContext;

/* Note: the field X->worksize now represents the task size in bytes */

#ifdef CONFIG_INIT_STACKS
	k_memset(X->workspace, 0xaa, X->worksize);
#endif

	contextOptions = 0;
	_START_TASK_ARCH(X, &contextOptions);

	/*
	 * The 'func' argument to _NewContext() represents the entry point of
	 * the
	 * kernel task.  The 'parameter1', 'parameter2', & 'parameter3'
	 * arguments
	 * are not applicable to such tasks.  A 'priority' of -1 indicates that
	 * the context is a task, rather than a fiber.
	 */

	pNewContext = (tCCS *)_NewContext((char *)X->workspace, /* pStackMem */
					  X->worksize,		/* stackSize */
					  (_ContextEntry)func,  /* pEntry */
					  (void *)0,		/* parameter1 */
					  (void *)0,		/* parameter2 */
					  (void *)0,		/* parameter3 */
					  -1,			/* priority */
					  contextOptions	/* options */
					  );

	X->workspace = (char *)pNewContext;
	X->fabort = NULL;

	reset_state_bit(X, TF_STOP | TF_TERM);
}

#endif /* CONFIG_MICROKERNEL */
