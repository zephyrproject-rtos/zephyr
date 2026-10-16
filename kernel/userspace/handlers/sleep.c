/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * System call verifiers for kernel/sleep.c, compiled only with CONFIG_USERSPACE.
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>

static inline k_ticks_t z_vrfy_k_sleep_ticks(k_timeout_t timeout)
{
	return z_impl_k_sleep_ticks(timeout);
}
#include <zephyr/syscalls/k_sleep_ticks_mrsh.c>
