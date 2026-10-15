/*
 * Copyright (c) 2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * System call verifiers for lib/os/printk.c, compiled only with CONFIG_USERSPACE.
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>

#ifdef CONFIG_PRINTK
static inline void z_vrfy_k_str_out(char *c, size_t n)
{
	K_OOPS(K_SYSCALL_MEMORY_READ(c, n));
	z_impl_k_str_out((char *)c, n);
}
#include <zephyr/syscalls/k_str_out_mrsh.c>
#endif /* CONFIG_PRINTK */
