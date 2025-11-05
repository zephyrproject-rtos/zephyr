/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief DSPIC k_thread_abort() routine
 *
 * thread abort needs to be called with IRQ locked, there is chance for the
 * thread to be switched out before it completes abort call. This can happen in
 * the event of a thread performing a self abort and an interrupt happens in
 * between.
 */

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <kswap.h>
#include <zephyr/sys/__assert.h>

void z_impl_k_thread_abort(k_tid_t thread)
{
	struct k_spinlock lock;
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_thread, abort, thread);

	k_spinlock_key_t key = k_spin_lock(&lock);

	z_thread_abort(thread);
	k_spin_unlock(&_sched_spinlock, key);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_thread, abort, thread);
}
