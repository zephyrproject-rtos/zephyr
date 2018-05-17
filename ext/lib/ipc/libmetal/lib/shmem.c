/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	generic/shmem.c
 * @brief	Generic libmetal shared memory handling.
 */

#include <errno.h>
#include <metal/assert.h>
#include <metal/shmem.h>
#include <metal/sys.h>
#include <metal/utilities.h>

int metal_shmem_register_generic(struct metal_generic_shmem *shmem)
{
	/* Make sure that we can be found. */
	metal_assert(shmem->name && strlen(shmem->name) != 0);

	/* Statically registered shmem regions cannot have a destructor. */
	metal_assert(!shmem->io.ops.close);

	metal_list_add_tail(&_metal.common.generic_shmem_list,
			    &shmem->node);
	return 0;
}

int metal_shmem_open_generic(const char *name, size_t size,
			     struct metal_io_region **result)
{
	struct metal_generic_shmem *shmem;
	struct metal_list *node;

	metal_list_for_each(&_metal.common.generic_shmem_list, node) {
		shmem = metal_container_of(node, struct metal_generic_shmem, node);
		if (strcmp(shmem->name, name) != 0)
			continue;
		if (size > metal_io_region_size(&shmem->io))
			continue;
		*result = &shmem->io;
		return 0;
	}

	return -ENOENT;
}
