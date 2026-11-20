/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * System call verifiers for kernel/scheduler.c, compiled only with CONFIG_USERSPACE.
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>
#include <ksched.h>

static inline void z_vrfy_k_reschedule(void)
{
	z_impl_k_reschedule();
}
#include <zephyr/syscalls/k_reschedule_mrsh.c>

static inline k_tid_t z_vrfy_k_sched_current_thread_query(void)
{
	return z_impl_k_sched_current_thread_query();
}
#include <zephyr/syscalls/k_sched_current_thread_query_mrsh.c>
