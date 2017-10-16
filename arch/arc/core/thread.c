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
	u32_t pc;
	u32_t status32;
	u32_t r3;
	u32_t r2;
	u32_t r1;
	u32_t r0;
};

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
void _new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		 size_t stackSize, k_thread_entry_t pEntry,
		 void *parameter1, void *parameter2, void *parameter3,
		 int priority, unsigned int options)
{
	char *pStackMem = K_THREAD_STACK_BUFFER(stack);
	_ASSERT_VALID_PRIO(priority, pEntry);

	char *stackEnd = pStackMem + stackSize;
	struct init_stack_frame *pInitCtx;

	_new_thread_init(thread, pStackMem, stackSize, priority, options);

	/* carve the thread entry struct from the "base" of the stack */
	pInitCtx = (struct init_stack_frame *)(STACK_ROUND_DOWN(stackEnd) -
				       sizeof(struct init_stack_frame));

	pInitCtx->pc = ((u32_t)_thread_entry_wrapper);
	pInitCtx->r0 = (u32_t)pEntry;
	pInitCtx->r1 = (u32_t)parameter1;
	pInitCtx->r2 = (u32_t)parameter2;
	pInitCtx->r3 = (u32_t)parameter3;
	/*
	 * For now set the interrupt priority to 15
	 * we can leave interrupt enable flag set to 0 as
	 * seti instruction in the end of the _Swap() will
	 * enable the interrupts based on intlock_key
	 * value.
	 */
#ifdef CONFIG_ARC_STACK_CHECKING
	pInitCtx->status32 = _ARC_V2_STATUS32_SC | _ARC_V2_STATUS32_E(_ARC_V2_DEF_IRQ_LEVEL);
	thread->arch.stack_base = (u32_t) stackEnd;
#else
	pInitCtx->status32 = _ARC_V2_STATUS32_E(_ARC_V2_DEF_IRQ_LEVEL);
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
		(u32_t)pInitCtx - ___callee_saved_stack_t_SIZEOF;

	/* initial values in all other regs/k_thread entries are irrelevant */

	thread_monitor_init(thread);
}
