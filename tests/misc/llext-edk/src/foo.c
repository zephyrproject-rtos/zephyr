/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <app_api.h>

#include <zephyr/llext/symbol.h>
#include <zephyr/internal/syscall_handler.h>

int z_impl_foo(int bar)
{
	return bar * bar;
}
EXPORT_SYMBOL(z_impl_foo);

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_foo(int bar)
{
	/* Nothing to verify */
	return z_impl_foo(bar);
}
#include <zephyr/syscalls/foo_mrsh.c>
#endif
