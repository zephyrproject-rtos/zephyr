/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * System call verifiers for kernel/busy_wait.c, compiled only with CONFIG_USERSPACE.
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>

static inline void z_vrfy_k_busy_wait(uint32_t usec_to_wait)
{
	z_impl_k_busy_wait(usec_to_wait);
}
#include <zephyr/syscalls/k_busy_wait_mrsh.c>
