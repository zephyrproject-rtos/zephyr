/*
 * Copyright (c) 2017, Linaro Limited. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	zephyr/shmem.c
 * @brief	Zephyr libmetal shared memory handling.
 */

#include <metal/shmem.h>

int metal_shmem_open(const char *name, size_t size,
		     struct metal_io_region **io)
{
	return metal_shmem_open_generic(name, size, io);
}
