/*
 * Copyright (c) 2018, Pinecone Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	nuttx/shmem.c
 * @brief	NuttX libmetal shared memory handling.
 */

#include <metal/shmem.h>

int metal_shmem_open(const char *name, size_t size,
		     struct metal_io_region **io)
{
	return metal_shmem_open_generic(name, size, io);
}
