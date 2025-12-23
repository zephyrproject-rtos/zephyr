/* memmap_b.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "master.h"
#include <zephyr/kernel.h>

/**
 * @brief Memory map get/free test
 */
void memorymap_test(void)
{
	uint64_t et; /* elapsed time */
	timing_t  start;
	timing_t  end;
	int i;
	void *p;
	int alloc_status;

	PRINT_STRING(dashline);
	start = timing_timestamp_get();
	for (i = 0; i < NR_OF_MAP_RUNS; i++) {
		alloc_status = k_mem_slab_alloc(&MAP1, &p, K_FOREVER);
		if (alloc_status != 0) {
			PRINT_F(FORMAT,
				"Error: Slab allocation failed.", (int64_t)alloc_status);
			break;
		}
		k_mem_slab_free(&MAP1, p);
	}
	end = timing_timestamp_get();
	et = timing_cycles_get(&start, &end);

	PRINT_F(FORMAT, "average alloc and dealloc memory page",
		timing_cycles_to_ns_avg(et, (2 * NR_OF_MAP_RUNS)));
}
