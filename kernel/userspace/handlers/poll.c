/*
 * Copyright (c) 2017 Wind River Systems, Inc.
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * System call verifiers for kernel/poll.c, compiled only with CONFIG_USERSPACE.
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>

static inline void z_vrfy_k_poll_signal_init(struct k_poll_signal *sig)
{
	K_OOPS(K_SYSCALL_OBJ_INIT(sig, K_OBJ_POLL_SIGNAL));
	z_impl_k_poll_signal_init(sig);
}
#include <zephyr/syscalls/k_poll_signal_init_mrsh.c>

void z_vrfy_k_poll_signal_check(struct k_poll_signal *sig,
			       unsigned int *signaled, int *result)
{
	K_OOPS(K_SYSCALL_OBJ(sig, K_OBJ_POLL_SIGNAL));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(signaled, sizeof(unsigned int)));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(result, sizeof(int)));
	z_impl_k_poll_signal_check(sig, signaled, result);
}
#include <zephyr/syscalls/k_poll_signal_check_mrsh.c>

static inline int z_vrfy_k_poll_signal_raise(struct k_poll_signal *sig,
					     int result)
{
	K_OOPS(K_SYSCALL_OBJ(sig, K_OBJ_POLL_SIGNAL));
	return z_impl_k_poll_signal_raise(sig, result);
}
#include <zephyr/syscalls/k_poll_signal_raise_mrsh.c>

static inline void z_vrfy_k_poll_signal_reset(struct k_poll_signal *sig)
{
	K_OOPS(K_SYSCALL_OBJ(sig, K_OBJ_POLL_SIGNAL));
	z_impl_k_poll_signal_reset(sig);
}
#include <zephyr/syscalls/k_poll_signal_reset_mrsh.c>
