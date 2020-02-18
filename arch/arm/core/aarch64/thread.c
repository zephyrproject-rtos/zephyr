/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief New thread creation for ARM64 Cortex-A
 *
 * Core thread related primitives for the ARM64 Cortex-A
 */

#include <kernel.h>
#include <ksched.h>
#include <wait_q.h>
#include <arch/cpu.h>

/**
 *
 * @brief Initialize a new thread from its stack space
 *
 * The control structure (thread) is put at the lower address of the stack. An
 * initial context, to be "restored" by z_arm64_context_switch(), is put at the
 * other end of the stack, and thus reusable by the stack when not needed
 * anymore.
 *
 * <options> is currently unused.
 *
 * @param stack      pointer to the aligned stack memory
 * @param stackSize  size of the available stack memory in bytes
 * @param pEntry the entry point
 * @param parameter1 entry point to the first param
 * @param parameter2 entry point to the second param
 * @param parameter3 entry point to the third param
 * @param priority   thread priority
 * @param options    thread options: K_ESSENTIAL, K_FP_REGS
 *
 * @return N/A
 */

void z_thread_entry_wrapper(k_thread_entry_t k, void *p1, void *p2, void *p3);

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     size_t stackSize, k_thread_entry_t pEntry,
		     void *parameter1, void *parameter2, void *parameter3,
		     int priority, unsigned int options)
{
	char *pStackMem = Z_THREAD_STACK_BUFFER(stack);
	char *stackEnd;
	struct __esf *pInitCtx;

	stackEnd = pStackMem + stackSize;

	z_new_thread_init(thread, pStackMem, stackSize, priority, options);

	pInitCtx = (struct __esf *)(STACK_ROUND_DOWN(stackEnd -
				    sizeof(struct __basic_sf)));

	pInitCtx->basic.regs[0] = (u64_t)pEntry;
	pInitCtx->basic.regs[1] = (u64_t)parameter1;
	pInitCtx->basic.regs[2] = (u64_t)parameter2;
	pInitCtx->basic.regs[3] = (u64_t)parameter3;

	/*
	 * We are saving:
	 *
	 * - SP: to pop out pEntry and parameters when going through
	 *   z_thread_entry_wrapper().
	 * - x30: to be used by ret in z_arm64_context_switch() when the new
	 *   task is first scheduled.
	 * - ELR_EL1: to be used by eret in z_thread_entry_wrapper() to return
	 *   to z_thread_entry() with pEntry in x0 and the parameters already
	 *   in place in x1, x2, x3.
	 * - SPSR_EL1: to enable IRQs (we are masking debug exceptions, SError
	 *   interrupts and FIQs).
	 */

	thread->callee_saved.sp = (u64_t)pInitCtx;
	thread->callee_saved.x30 = (u64_t)z_thread_entry_wrapper;
	thread->callee_saved.elr = (u64_t)z_thread_entry;
	thread->callee_saved.spsr = SPSR_MODE_EL1H | DAIF_FIQ;
}
