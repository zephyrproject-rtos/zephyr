/* context.c - new context creation for ARCv2 */

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
 * DESCRIPTION
 * Core nanokernel fiber related primitives for the ARCv2 processor
 * architecture.
 */

#include <nanokernel.h>
#include <nanokernel/cpu.h>
#include <toolchain.h>
#include <nanok.h>
#include <nanocontextentry.h>
#include <offsets.h>

/*  initial stack frame */
struct init_stack_frame {
	uint32_t pc;
	uint32_t status32;
	uint32_t r3;
	uint32_t r2;
	uint32_t r1;
	uint32_t r0;
};

tNANO _NanoKernel = {0};

#if defined(CONFIG_CONTEXT_MONITOR)
#define CONTEXT_MONITOR_INIT(pCcs) context_monitor_init(pCcs)
#else
#define CONTEXT_MONITOR_INIT(pCcs) \
	do {/* do nothing */     \
	} while ((0))
#endif

#if defined(CONFIG_CONTEXT_MONITOR)
/*
 * context_monitor_init - initialize context monitoring support
 *
 * Currently only inserts the new context in the list of active contexts.
 *
 * RETURNS: N/A
 */

static ALWAYS_INLINE void context_monitor_init(struct s_CCS *pCcs /* context */
					   )
{
	unsigned int key;

	/*
	 * Add the newly initialized context to head of the list of contexts.
	 * This singly linked list of contexts maintains ALL the contexts in the
	 * system: both tasks and fibers regardless of whether they are
	 * runnable.
	 */

	key = irq_lock_inline();
	pCcs->next_context = _NanoKernel.contexts;
	_NanoKernel.contexts = pCcs;
	irq_unlock_inline(key);
}
#endif /* CONFIG_CONTEXT_MONITOR */

/*
 * _NewContext - initialize a new context (thread) from its stack space
 *
 * The control structure (CCS) is put at the lower address of the stack. An
 * initial context, to be "restored" by __return_from_coop(), is put at
 * the other end of the stack, and thus reusable by the stack when not
 * needed anymore.
 *
 * The initial context is a basic stack frame that contains arguments for
 * _context_entry() return address, that points at _context_entry()
 * and status register.
 *
 * <options> is currently unused.
 *
 * RETURNS: N/A
 */

void *_NewContext(
	char *pStackMem,       /* pointer to stack memory */
	unsigned stackSize,    /* stack size in bytes */
	_ContextEntry pEntry,  /* context (thread) entry point routine */
	void *parameter1,      /* first param to entry point */
	void *parameter2,      /* second param to entry point */
	void *parameter3,      /* third param to entry point */
	int priority,          /* fiber priority, -1 for task */
	unsigned options       /* unused, for expansion */
)
{
	char *stackEnd = pStackMem + stackSize;
	struct init_stack_frame *pInitCtx;

	tCCS *pCcs = (void *)ROUND_UP(pStackMem, sizeof(uint32_t));

	/* carve the context entry struct from the "base" of the stack */

	pInitCtx = (struct init_stack_frame *)(STACK_ROUND_DOWN(stackEnd) -
				       sizeof(struct init_stack_frame));

	pInitCtx->pc = ((uint32_t)_ContextEntryWrapper);
	pInitCtx->r0 = (uint32_t)pEntry;
	pInitCtx->r1 = (uint32_t)parameter1;
	pInitCtx->r2 = (uint32_t)parameter2;
	pInitCtx->r3 = (uint32_t)parameter3;
	/*
	 * For now set the interrupt priority to 15
	 * we can leave interrupt enable flag set to 0 as
	 * seti instruction in the end of the _Swap() will
	 * enable the interrupts based on intlock_key
	 * value.
	 */
	pInitCtx->status32 = _ARC_V2_STATUS32_E(_ARC_V2_DEF_IRQ_LEVEL);

	pCcs->link = NULL;
	pCcs->flags = priority == -1 ? TASK | PREEMPTIBLE : FIBER;
	pCcs->prio = priority;

#ifdef CONFIG_CONTEXT_CUSTOM_DATA
	/* Initialize custom data field (value is opaque to kernel) */

	pCcs->custom_data = NULL;
#endif

	/*
	 * intlock_key is constructed based on ARCv2 ISA Programmer's
	 * Reference Manual CLRI instruction description:
	 * dst[31:6] dst[5] dst[4]       dst[3:0]
	 *    26'd0    1    STATUS32.IE  STATUS32.E[3:0]
	 */
	pCcs->intlock_key = 0x3F;
	pCcs->relinquish_cause = _CAUSE_COOP;
	pCcs->preempReg.sp = (uint32_t)pInitCtx - __tCalleeSaved_SIZEOF;

	/* initial values in all other registers/CCS entries are irrelevant */

	CONTEXT_MONITOR_INIT(pCcs);

	return pCcs;
}
