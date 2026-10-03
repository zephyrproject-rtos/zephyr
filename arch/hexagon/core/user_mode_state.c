/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Hexagon user mode state tracking
 *
 * arch_is_user_context() cannot read struct k_thread directly because
 * syscall.h is processed before struct k_thread is fully defined. Instead,
 * a per-system volatile flag tracks the current thread's privilege level,
 * set by arch_user_mode_enter() and kept in sync by the trap0 handler on
 * every kernel re-entry/exit. Correct as long as at most one user-mode
 * thread is active at a time (true today) and the update happens
 * atomically with the mode switch (guest interrupts are disabled during
 * event handling).
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/hexagon/arch.h>

#ifdef CONFIG_USERSPACE

BUILD_ASSERT(!IS_ENABLED(CONFIG_SMP),
	     "Hexagon user mode state uses a global flag: SMP is not supported");

/* Nonzero when the current thread is executing in user mode */
volatile uint32_t _hexagon_user_mode_active;

/**
 * @brief Synchronise the global flag with the current thread's priv_level.
 *
 * Called from the trap0/exception handler just before vmrte, so
 * arch_is_user_context() returns the right value for the thread about to
 * run.
 */
void z_hexagon_user_mode_sync(void)
{
	_hexagon_user_mode_active = (_current->arch.priv_level != 0) ? 1U : 0U;
}

#endif /* CONFIG_USERSPACE */
