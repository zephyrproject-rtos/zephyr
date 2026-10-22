/*
 * Copyright (c) 2026 Andy Lin <andylinpersonal@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <stdint.h>

typedef int (*fn_iPmmm_t)(uint32_t *, uint32_t, uint32_t);

int z_impl_sys_wrapper_iPmmm(fn_iPmmm_t fn, int *r, uint32_t *a, uint32_t b, uint32_t c)
{
	if (fn == NULL) {
		return -EINVAL;
	}

	int ret = fn(a, b, c);

	if (r != NULL) {
		*r = ret;
	}

	return 0;
}

#if CONFIG_USERSPACE
int z_vrfy_sys_wrapper_iPmmm(fn_iPmmm_t fn, int *r, uint32_t *a, uint32_t b, uint32_t c)
{
	return z_impl_sys_wrapper_iPmmm(fn, r, a, b, c);
}

#include <zephyr/syscalls/sys_wrapper_iPmmm_mrsh.c>
#endif
