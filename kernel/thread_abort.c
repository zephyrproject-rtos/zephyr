/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Primitive for aborting a thread when an arch-specific one is not
 * needed..
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <nano_internal.h>
#include <string.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <wait_q.h>
#include <ksched.h>

extern void _k_thread_single_abort(struct k_thread *thread);

#if !defined(CONFIG_ARCH_HAS_THREAD_ABORT)
void k_thread_abort(k_tid_t thread)
{
	unsigned int key;

	key = irq_lock();

	_k_thread_single_abort(thread);
	_thread_monitor_exit(thread);

	if (_current == thread) {
		_Swap(key);
		CODE_UNREACHABLE;
	}

	/* The abort handler might have altered the ready queue. */
	_reschedule_threads(key);
}
#endif
