/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief New thread creation for ARMv5 (ARM9 / XScale)
 *
 * A thread is born through the exception return path: arch_new_thread() lays
 * an exception stack frame at the top of the thread stack and points the
 * thread's saved lr at z_armv5_exit_exc(), so the very first context switch
 * into the thread "returns from an exception" straight into z_thread_entry().
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/arch/arm/arm9/cpu.h>
#include <stdbool.h>
#include <errno.h>

extern void z_armv5_exit_exc(void);

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack, char *stack_ptr,
		     k_thread_entry_t entry, void *p1, void *p2, void *p3)
{
	struct __basic_sf *iframe;

	iframe = Z_STACK_PTR_TO_FRAME(struct __basic_sf, stack_ptr);

	iframe->pc = (uint32_t)z_thread_entry;
	iframe->a1 = (uint32_t)entry;
	iframe->a2 = (uint32_t)p1;
	iframe->a3 = (uint32_t)p2;
	iframe->a4 = (uint32_t)p3;
	iframe->lr = 0U;
	iframe->ip = 0U;

	/*
	 * Threads run in system mode with imprecise aborts masked. ARMv5 has
	 * no Thumb-2, so a Zephyr build is always ARM state and T_BIT stays
	 * clear.
	 */
	iframe->xpsr = A_BIT | MODE_SYS;
#if defined(CONFIG_BIG_ENDIAN)
	iframe->xpsr |= E_BIT;
#endif

	thread->callee_saved.psp = (uint32_t)iframe;
	thread->arch.basepri = 0;
	thread->arch.swap_return_value = 0;

	thread->switch_handle = thread;
	/* thread birth happens through the exception return path */
	thread->arch.exception_depth = 1;
	thread->callee_saved.lr = (uint32_t)z_armv5_exit_exc;
}

int arch_coprocessors_disable(struct k_thread *thread)
{
	ARG_UNUSED(thread);

	/*
	 * There is no per-thread coprocessor state to drop: this port has no
	 * VFP support and leaves the XScale DSP coprocessor disabled.
	 */
	return -ENOTSUP;
}
