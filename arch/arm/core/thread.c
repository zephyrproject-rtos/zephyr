/* thread.c - new thread creation for ARM Cortex-M */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
#ifdef CONFIG_INIT_STACKS
#include <string.h>
#endif /* CONFIG_INIT_STACKS */

tNANO _nanokernel = {0};

#if defined(CONFIG_THREAD_MONITOR)
#define THREAD_MONITOR_INIT(tcs) _thread_monitor_init(tcs)
#else
#define THREAD_MONITOR_INIT(tcs) \
	do {/* do nothing */     \
	} while ((0))
#endif

#if defined(CONFIG_THREAD_MONITOR)
/**
 *
 * @brief Initialize thread monitoring support
 *
 * Currently only inserts the new thread in the list of active threads.
 *
 * @return N/A
 */

static ALWAYS_INLINE void _thread_monitor_init(struct tcs *tcs /* thread */
					   )
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

/**
 *
 * @brief Intialize a new thread from its stack space
 *
 * The control structure (TCS) is put at the lower address of the stack. An
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
 * @param pStackMem the aligned stack memory
 * @param stackSize stack size in bytes
 * @param pEntry the entry point
 * @param parameter1 entry point to the first param
 * @param parameter2 entry point to the second param
 * @param parameter3 entry point to the third param
 * @param priority thread priority (-1 for tasks)
 * @param misc options (future use)
 *
 * @return N/A
 */

void _new_thread(char *pStackMem, unsigned stackSize, _thread_entry_t pEntry,
		 void *parameter1, void *parameter2, void *parameter3,
		 int priority, unsigned options)
{
	char *stackEnd = pStackMem + stackSize;
	struct __esf *pInitCtx;
	struct tcs *tcs = (struct tcs *) pStackMem;

#ifdef CONFIG_INIT_STACKS
	memset(pStackMem, 0xaa, stackSize);
#endif

	/* carve the thread entry struct from the "base" of the stack */

	pInitCtx = (struct __esf *)(STACK_ROUND_DOWN(stackEnd) -
				    sizeof(struct __esf));

	pInitCtx->pc = ((uint32_t)_thread_entry) & 0xfffffffe;
	pInitCtx->a1 = (uint32_t)pEntry;
	pInitCtx->a2 = (uint32_t)parameter1;
	pInitCtx->a3 = (uint32_t)parameter2;
	pInitCtx->a4 = (uint32_t)parameter3;
	pInitCtx->xpsr =
		0x01000000UL; /* clear all, thumb bit is 1, even if RO */

	tcs->link = NULL;
	tcs->flags = priority == -1 ? TASK | PREEMPTIBLE : FIBER;
	tcs->prio = priority;

#ifdef CONFIG_THREAD_CUSTOM_DATA
	/* Initialize custom data field (value is opaque to kernel) */

	tcs->custom_data = NULL;
#endif

	tcs->preempReg.psp = (uint32_t)pInitCtx;
	tcs->basepri = 0;

	_nano_timeout_tcs_init(tcs);

	/* initial values in all other registers/TCS entries are irrelevant */

	THREAD_MONITOR_INIT(tcs);
}
