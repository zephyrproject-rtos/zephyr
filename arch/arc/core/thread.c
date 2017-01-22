/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief New thread creation for ARCv2
 *
 * Core thread related primitives for the ARCv2 processor architecture.
 */

#include <kernel.h>
#include <toolchain.h>
#include <kernel_structs.h>
#include <offsets_short.h>
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

#if defined(CONFIG_THREAD_MONITOR)
/*
 * Add a thread to the kernel's list of active threads.
 */
static ALWAYS_INLINE void thread_monitor_init(struct k_thread *thread)
{
	unsigned int key;

	key = irq_lock();
	thread->next_thread = _kernel.threads;
	_kernel.threads = thread;
	irq_unlock(key);
}
#else
#define thread_monitor_init(thread) \
	do {/* do nothing */     \
	} while ((0))
#endif /* CONFIG_THREAD_MONITOR */

/*
 * @brief Initialize a new thread from its stack space
 *
 * The thread control structure is put at the lower address of the stack. An
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
 * @param options thread options: K_ESSENTIAL
 *
 * @return N/A
 */
void _new_thread(char *pStackMem, size_t stackSize,
		 _thread_entry_t pEntry,
		 void *parameter1, void *parameter2, void *parameter3,
		 int priority, unsigned options)
{
	_ASSERT_VALID_PRIO(priority, pEntry);

	char *stackEnd = pStackMem + stackSize;
	struct init_stack_frame *pInitCtx;

	struct k_thread *thread = (struct k_thread *) pStackMem;

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
	thread->arch.stack_top = (uint32_t) stackEnd;
#else
	pInitCtx->status32 = _ARC_V2_STATUS32_E(_ARC_V2_DEF_IRQ_LEVEL);
#endif

	_init_thread_base(&thread->base, priority, _THREAD_PRESTART, options);

	/* static threads overwrite them afterwards with real values */
	thread->init_data = NULL;
	thread->fn_abort = NULL;

#ifdef CONFIG_THREAD_CUSTOM_DATA
	/* Initialize custom data field (value is opaque to kernel) */

	thread->custom_data = NULL;
#endif

#ifdef CONFIG_THREAD_MONITOR
	/*
	 * In debug mode thread->entry give direct access to the thread entry
	 * and the corresponding parameters.
	 */
	thread->entry = (struct __thread_entry *)(pInitCtx);
#endif

	/*
	 * intlock_key is constructed based on ARCv2 ISA Programmer's
	 * Reference Manual CLRI instruction description:
	 * dst[31:6] dst[5] dst[4]       dst[3:0]
	 *    26'd0    1    STATUS32.IE  STATUS32.E[3:0]
	 */
	thread->arch.intlock_key = 0x3F;
	thread->arch.relinquish_cause = _CAUSE_COOP;
	thread->callee_saved.sp =
		(uint32_t)pInitCtx - ___callee_saved_stack_t_SIZEOF;

	/* initial values in all other regs/k_thread entries are irrelevant */

	thread_monitor_init(thread);
}
