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

struct init_stack_frame {
	/* top of the stack / most recently pushed */

	/* SPSL_ELn and ELR_ELn */
	uint64_t spsr;
	uint64_t elr;

	/*
	 * Used by z_thread_entry_wrapper. pulls these off the stack and
	 * into argument registers before calling z_thread_entry()
	 */
	uint64_t entry_point;
	uint64_t arg1;
	uint64_t arg2;
	uint64_t arg3;

	/* least recently pushed */
};

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     size_t stackSize, k_thread_entry_t pEntry,
		     void *parameter1, void *parameter2, void *parameter3,
		     int priority, unsigned int options)
{
	char *pStackMem = Z_THREAD_STACK_BUFFER(stack);
	char *stackEnd;
	struct init_stack_frame *pInitCtx;

	stackEnd = pStackMem + stackSize;

	z_new_thread_init(thread, pStackMem, stackSize);

	pInitCtx = (struct init_stack_frame *)(Z_STACK_PTR_ALIGN(stackEnd -
				    sizeof(struct init_stack_frame)));

	pInitCtx->entry_point = (uint64_t)pEntry;
	pInitCtx->arg1 = (uint64_t)parameter1;
	pInitCtx->arg2 = (uint64_t)parameter2;
	pInitCtx->arg3 = (uint64_t)parameter3;

	/*
	 * - ELR_ELn: to be used by eret in z_thread_entry_wrapper() to return
	 *   to z_thread_entry() with pEntry in x0(entry_point) and the parameters
	 *   already in place in x1(arg1), x2(arg2), x3(arg3).
	 * - SPSR_ELn: to enable IRQs (we are masking debug exceptions, SError
	 *   interrupts and FIQs).
	 */
	pInitCtx->elr = (uint64_t)z_thread_entry;
	pInitCtx->spsr = SPSR_MODE_EL1H | DAIF_FIQ;

	/*
	 * We are saving:
	 *
	 * - SP: to pop out pEntry and parameters when going through
	 *   z_thread_entry_wrapper().
	 * - x30: to be used by ret in z_arm64_context_switch() when the new
	 *   task is first scheduled.
	 */

	thread->callee_saved.sp = (uint64_t)pInitCtx;
	thread->callee_saved.x30 = (uint64_t)z_thread_entry_wrapper;
}
