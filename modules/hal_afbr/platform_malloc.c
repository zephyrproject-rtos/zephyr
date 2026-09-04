/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/check.h>

#define DT_DRV_COMPAT		brcm_afbr_s50
#define NUM_AFBR_INST		DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)

BUILD_ASSERT(NUM_AFBR_INST > 0, "Invalid number of AFBR-S50 instances");

/** Defined separate memslab to isolate the library from the other
 * components. The library's device handle is a single allocation of
 * ~4.1-KiB per instance, so the blocks have to be large enough to satisfy
 * it; a request larger than a block fails honestly instead of handing
 * back undersized memory.
 */
#define ARGUS_ALLOC_BLOCK_SIZE	4352

K_MEM_SLAB_DEFINE(argus_memslab, ARGUS_ALLOC_BLOCK_SIZE, 2 * NUM_AFBR_INST,
		  sizeof(void *));

void *Argus_Malloc(size_t size)
{
	void *ptr = NULL;
	int err;

	CHECKIF(size > ARGUS_ALLOC_BLOCK_SIZE) {
		return NULL;
	}

	err = k_mem_slab_alloc(&argus_memslab, &ptr, K_NO_WAIT);

	CHECKIF(err != 0 || ptr == NULL) {
		return NULL;
	}

	return ptr;
}

void Argus_Free(void *ptr)
{
	k_mem_slab_free(&argus_memslab, ptr);
}
