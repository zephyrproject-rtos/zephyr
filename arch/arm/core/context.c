/* context.c - new context creation for ARM Cortex-M */

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
Core nanokernel fiber related primitives for the ARM Cortex-M processor
architecture.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <toolchain.h>
#include <nano_private.h>
#include <wait_q.h>

tNANO _nanokernel = {0};

#if defined(CONFIG_CONTEXT_MONITOR)
#define CONTEXT_MONITOR_INIT(pCcs) _context_monitor_init(pCcs)
#else
#define CONTEXT_MONITOR_INIT(pCcs) \
	do {/* do nothing */     \
	} while ((0))
#endif

#if defined(CONFIG_CONTEXT_MONITOR)
/**
 *
 * @brief Initialize context monitoring support
 *
 * Currently only inserts the new context in the list of active contexts.
 *
 * @return N/A
 */

static ALWAYS_INLINE void _context_monitor_init(struct ccs *pCcs /* context */
					   )
{
	unsigned int key;

	/*
	 * Add the newly initialized context to head of the list of contexts.
	 * This singly linked list of contexts maintains ALL the contexts in the
	 * system: both tasks and fibers regardless of whether they are
	 * runnable.
	 */

	key = irq_lock();
	pCcs->next_context = _nanokernel.contexts;
	_nanokernel.contexts = pCcs;
	irq_unlock(key);
}
#endif /* CONFIG_CONTEXT_MONITOR */

/**
 *
 * @brief Intialize a new context (thread) from its stack space
 *
 * The control structure (CCS) is put at the lower address of the stack. An
 * initial context, to be "restored" by __pendsv(), is put at the other end of
 * the stack, and thus reusable by the stack when not needed anymore.
 *
 * The initial context is an exception stack frame (ESF) since exiting the
 * PendSV exception will want to pop an ESF. Interestingly, even if the lsb of
 * an instruction address to jump to must always be set since the CPU always
 * runs in thumb mode, the ESF expects the real address of the instruction,
 * with the lsb *not* set (instructions are always aligned on 16 bit halfwords).
 * Since the compiler automatically sets the lsb of function addresses, we have
 * to unset it manually before storing it in the 'pc' field of the ESF.
 *
 * <options> is currently unused.
 *
 * @return N/A
 */

void _NewContext(
	char *pStackMem,      /* aligned stack memory */
	unsigned stackSize,   /* stack size in bytes */
	_ContextEntry pEntry, /* entry point */
	void *parameter1,     /* entry point first param */
	void *parameter2,     /* entry point second param */
	void *parameter3,     /* entry point third param */
	int priority,	 /* context priority (-1 for tasks) */
	unsigned options      /* misc options (future) */
	)
{
	char *stackEnd = pStackMem + stackSize;
	struct __esf *pInitCtx;
	tCCS *pCcs = (tCCS *) pStackMem;

#ifdef CONFIG_INIT_STACKS
    k_memset(pStackMem, 0xaa, stackSize);
#endif

	/* carve the context entry struct from the "base" of the stack */

	pInitCtx = (struct __esf *)(STACK_ROUND_DOWN(stackEnd) -
				    sizeof(struct __esf));

	pInitCtx->pc = ((uint32_t)_context_entry) & 0xfffffffe;
	pInitCtx->a1 = (uint32_t)pEntry;
	pInitCtx->a2 = (uint32_t)parameter1;
	pInitCtx->a3 = (uint32_t)parameter2;
	pInitCtx->a4 = (uint32_t)parameter3;
	pInitCtx->xpsr =
		0x01000000UL; /* clear all, thumb bit is 1, even if RO */

	pCcs->link = NULL;
	pCcs->flags = priority == -1 ? TASK | PREEMPTIBLE : FIBER;
	pCcs->prio = priority;

#ifdef CONFIG_CONTEXT_CUSTOM_DATA
	/* Initialize custom data field (value is opaque to kernel) */

	pCcs->custom_data = NULL;
#endif

	pCcs->preempReg.psp = (uint32_t)pInitCtx;
	pCcs->basepri = 0;

	_nano_timeout_ccs_init(pCcs);

	/* initial values in all other registers/CCS entries are irrelevant */

	CONTEXT_MONITOR_INIT(pCcs);
}
