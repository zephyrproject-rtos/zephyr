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
#include <kernel_internal.h>
#include <kswap.h>
#include <string.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <wait_q.h>
#include <ksched.h>
#include <misc/__assert.h>
#include <syscall_handler.h>

extern void _k_thread_single_abort(struct k_thread *thread);

#if !defined(CONFIG_ARCH_HAS_THREAD_ABORT)
void _impl_k_thread_abort(k_tid_t thread)
{
	/* We aren't trying to synchronize data access here (these
	 * APIs are internally synchronized).  The original lock seems
	 * to have been in place to prevent the thread from waking up
	 * due to a delivered interrupt.  Leave a dummy spinlock in
	 * place to do that.  This API should be revisted though, it
	 * doesn't look SMP-safe as it stands.
	 */
	struct k_spinlock lock = {};
	k_spinlock_key_t key = k_spin_lock(&lock);

	__ASSERT((thread->base.user_options & K_ESSENTIAL) == 0,
		 "essential thread aborted");

	_k_thread_single_abort(thread);
	_thread_monitor_exit(thread);

	_reschedule(&lock, key);
}
#endif

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_thread_abort, thread_p)
{
	struct k_thread *thread = (struct k_thread *)thread_p;
	Z_OOPS(Z_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(!(thread->base.user_options & K_ESSENTIAL),
				    "aborting essential thread %p", thread));

	_impl_k_thread_abort((struct k_thread *)thread);
	return 0;
}
#endif
