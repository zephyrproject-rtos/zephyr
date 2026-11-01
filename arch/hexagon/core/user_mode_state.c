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
 * a thread-local volatile flag tracks the current thread's privilege level,
 * set by arch_user_mode_enter() and kept in sync by the trap0 handler on
 * every kernel re-entry/exit. Correct as long as the update happens
 * atomically with the mode switch (guest interrupts are disabled during
 * event handling).
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/hexagon/arch.h>

#ifdef CONFIG_USERSPACE

/* Nonzero when the current thread is executing in user mode */
Z_THREAD_LOCAL volatile uint32_t _hexagon_user_mode_active;

/**
 * @brief Synchronise the thread-local flag with the current thread's priv_level.
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
