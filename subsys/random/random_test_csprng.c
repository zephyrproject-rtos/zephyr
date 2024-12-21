/*
 * Copyright (c) 2024 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/random/random.h>

int z_impl_sys_csrand_get(void *dst, size_t outlen)
{
	sys_rand_get(dst, outlen);
	return 0;
}
