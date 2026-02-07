/*
 * Copyright (c) 2025-2026, Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file thread.c
 * @brief New thread creation for ARM9
 */

#include <ksched.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/sys/barrier.h>

extern void z_arm_arm9_exit_exc(void);

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *p1, void *p2, void *p3)
{
	struct __basic_sf *iframe;

	iframe = Z_STACK_PTR_TO_FRAME(struct __basic_sf, stack_ptr);
	iframe->pc = (uint32_t)z_thread_entry;

	iframe->a1 = (uint32_t)entry;
	iframe->a2 = (uint32_t)p1;
	iframe->a3 = (uint32_t)p2;
	iframe->a4 = (uint32_t)p3;

	iframe->xpsr = MODE_SYS;

	thread->callee_saved.psp = (uint32_t)iframe;
	thread->arch.basepri = 0;

	/* initial values in all other registers/thread entries are irrelevant. */
	thread->switch_handle = thread;
	/* thread birth happens through the exception return path */
	thread->arch.exception_depth = 1;
	thread->callee_saved.lr = (uint32_t)z_arm_arm9_exit_exc;
}

int arch_coprocessors_disable(struct k_thread *thread)
{
	return -ENOTSUP;
}
