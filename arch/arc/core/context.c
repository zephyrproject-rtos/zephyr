/* thread.c - new thread creation for ARCv2 */

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
#include <arch/cpu.h>
#include <toolchain.h>
#include <nano_private.h>
#include <offsets.h>
#include <wait_q.h>
#ifdef CONFIG_INIT_STACKS
#include <string.h>
#endif /* CONFIG_INIT_STACKS */
/*  initial stack frame */
struct init_stack_frame {
	uint32_t pc;
	uint32_t status32;
	uint32_t r3;
	uint32_t r2;
	uint32_t r1;
	uint32_t r0;
};

tNANO _nanokernel = {0};

#if defined(CONFIG_THREAD_MONITOR)
#define THREAD_MONITOR_INIT(tcs) thread_monitor_init(tcs)
#else
#define THREAD_MONITOR_INIT(tcs) \
	do {/* do nothing */     \
	} while ((0))
#endif

#if defined(CONFIG_THREAD_MONITOR)
/*
 * @brief Initialize thread monitoring support
 *
 * Currently only inserts the new thread in the list of active threads.
 *
 * @return N/A
 */

static ALWAYS_INLINE void thread_monitor_init(struct tcs *tcs)
{
	unsigned int key;

	/*
	 * Add the newly initialized thread to head of the list of threads.  This
	 * singly linked list of threads maintains ALL the threads in the system:
	 * both tasks and fibers regardless of whether they are runnable.
	 */

	key = irq_lock();
	tcs->next_thread = _nanokernel.threads;
	_nanokernel.threads = tcs;
	irq_unlock(key);
}
#endif /* CONFIG_THREAD_MONITOR */

/*
 * @brief Initialize a new thread from its stack space
 *
 * The control structure (TCS) is put at the lower address of the stack. An
 * initial context, to be "restored" by __return_from_coop(), is put at
 * the other end of the stack, and thus reusable by the stack when not
 * needed anymore.
 *
 * The initial context is a basic stack frame that contains arguments for
 * _thread_entry() return address, that points at _thread_entry()
 * and status register.
 *
 * <options> is currently unused.
 *
 * @return N/A
 */

void _new_thread(
	char *pStackMem,       /* pointer to aligned stack memory */
	unsigned stackSize,    /* stack size in bytes */
	_thread_entry_t pEntry,  /* thread entry point routine */
	void *parameter1,      /* first param to entry point */
	void *parameter2,      /* second param to entry point */
	void *parameter3,      /* third param to entry point */
	int priority,          /* fiber priority, -1 for task */
	unsigned options       /* unused, for expansion */
)
{
	char *stackEnd = pStackMem + stackSize;
	struct init_stack_frame *pInitCtx;

	struct tcs *tcs = (struct tcs *) pStackMem;

#ifdef CONFIG_INIT_STACKS
	memset(pStackMem, 0xaa, stackSize);
#endif

	/* carve the thread entry struct from the "base" of the stack */

	pInitCtx = (struct init_stack_frame *)(STACK_ROUND_DOWN(stackEnd) -
				       sizeof(struct init_stack_frame));

	pInitCtx->pc = ((uint32_t)_thread_entry_wrapper);
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

	tcs->link = NULL;
	tcs->flags = priority == -1 ? TASK | PREEMPTIBLE : FIBER;
	tcs->prio = priority;

#ifdef CONFIG_THREAD_CUSTOM_DATA
	/* Initialize custom data field (value is opaque to kernel) */

	tcs->custom_data = NULL;
#endif

	/*
	 * intlock_key is constructed based on ARCv2 ISA Programmer's
	 * Reference Manual CLRI instruction description:
	 * dst[31:6] dst[5] dst[4]       dst[3:0]
	 *    26'd0    1    STATUS32.IE  STATUS32.E[3:0]
	 */
	tcs->intlock_key = 0x3F;
	tcs->relinquish_cause = _CAUSE_COOP;
	tcs->preempReg.sp = (uint32_t)pInitCtx - __tCalleeSaved_SIZEOF;

	_nano_timeout_tcs_init(tcs);

	/* initial values in all other registers/TCS entries are irrelevant */

	THREAD_MONITOR_INIT(tcs);
}
