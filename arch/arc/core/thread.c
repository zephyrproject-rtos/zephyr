/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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

/**
 * @file
 * @brief New thread creation for ARCv2
 *
 * Core nanokernel fiber related primitives for the ARCv2 processor
 * architecture.
 */

#include <nanokernel.h>
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
/*
 * Add a thread to the kernel's list of active threads.
 */
static ALWAYS_INLINE void thread_monitor_init(struct tcs *tcs)
{
	unsigned int key;

	key = irq_lock();
	tcs->next_thread = _nanokernel.threads;
	_nanokernel.threads = tcs;
	irq_unlock(key);
}
#else
#define thread_monitor_init(tcs) \
	do {/* do nothing */     \
	} while ((0))
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
 * @param pStackmem the pointer to aligned stack memory
 * @param stackSize the stack size in bytes
 * @param pEntry thread entry point routine
 * @param parameter1 first param to entry point
 * @param parameter2 second param to entry point
 * @param parameter3 third param to entry point
 * @param priority thread priority
 * @param options thread options: ESSENTIAL
 *
 * @return N/A
 */
void _new_thread(char *pStackMem, unsigned stackSize,
		 void *uk_task_ptr, _thread_entry_t pEntry,
		 void *parameter1, void *parameter2, void *parameter3,
		 int priority, unsigned options)
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
#ifdef CONFIG_ARC_STACK_CHECKING
	pInitCtx->status32 = _ARC_V2_STATUS32_SC | _ARC_V2_STATUS32_E(_ARC_V2_DEF_IRQ_LEVEL);
	tcs->stack_top = (uint32_t) stackEnd;
#else
	pInitCtx->status32 = _ARC_V2_STATUS32_E(_ARC_V2_DEF_IRQ_LEVEL);
#endif

#ifdef CONFIG_KERNEL_V2
	/* k_q_node initialized upon first insertion in a list */
	tcs->flags = options | K_PRESTART;
	tcs->sched_locked = 0;

	/* static threads overwrite them afterwards with real values */
	tcs->init_data = NULL;
	tcs->fn_abort = NULL;
#else
	tcs->link = NULL;
	tcs->flags = priority == -1 ? TASK | PREEMPTIBLE : FIBER;
#endif
	tcs->prio = priority;

#ifdef CONFIG_THREAD_CUSTOM_DATA
	/* Initialize custom data field (value is opaque to kernel) */

	tcs->custom_data = NULL;
#endif

#ifdef CONFIG_THREAD_MONITOR
	/*
	 * In debug mode tcs->entry give direct access to the thread entry
	 * and the corresponding parameters.
	 */
	tcs->entry = (struct __thread_entry *)(pInitCtx);
#endif

#if !defined(CONFIG_KERNEL_V2) && defined(CONFIG_MICROKERNEL)
	tcs->uk_task_ptr = uk_task_ptr;
#else
	ARG_UNUSED(uk_task_ptr);
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

	thread_monitor_init(tcs);
}
