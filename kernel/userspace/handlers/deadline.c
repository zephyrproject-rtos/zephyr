/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * System call verifiers for kernel/deadline.c, compiled only with CONFIG_USERSPACE.
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>
#include <ksched.h>

static inline void z_vrfy_k_thread_absolute_deadline_set(k_tid_t tid, int deadline)
{
	struct k_thread *thread = tid;

	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));

	z_impl_k_thread_absolute_deadline_set((k_tid_t)thread, deadline);
}
#include <zephyr/syscalls/k_thread_absolute_deadline_set_mrsh.c>

static inline void z_vrfy_k_thread_deadline_set(k_tid_t tid, int deadline)
{
	struct k_thread *thread = tid;

	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	K_OOPS(K_SYSCALL_VERIFY_MSG(deadline > 0,
				    "invalid thread deadline %d",
				    (int)deadline));

	z_impl_k_thread_deadline_set((k_tid_t)thread, deadline);
}
#include <zephyr/syscalls/k_thread_deadline_set_mrsh.c>
