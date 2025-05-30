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

/** Defined separate memslab to isolate library from the other components.
 * Through debugging, the library requests an initial allocation of ~4-KiB,
 * which is why the total pool is sized to 8-KiB per instance.
 */
K_MEM_SLAB_DEFINE(argus_memslab, 64, 128 * NUM_AFBR_INST, sizeof(void *));

void *Argus_Malloc(size_t size)
{
	void *ptr = NULL;
	int err;

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
