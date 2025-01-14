/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/ztest_assert.h>

int k_mem_slab_alloc(struct k_mem_slab *slab, void **mem, k_timeout_t timeout)
{
	if (slab->info.num_used >= slab->info.num_blocks) {
		*mem = NULL;
		return -ENOMEM;
	}

	*mem = malloc(slab->info.block_size);
	zassert_not_null(*mem);

	slab->info.num_used++;
	return 0;
}

void k_mem_slab_free(struct k_mem_slab *slab, void *mem)
{
	free(mem);
	slab->info.num_used--;
}
