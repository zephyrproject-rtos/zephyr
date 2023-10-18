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
	uint32_t et; /* elapsed time */
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
				"Error: Slab allocation failed.", alloc_status);
			break;
		}
		k_mem_slab_free(&MAP1, p);
	}
	end = timing_timestamp_get();
	et = (uint32_t)timing_cycles_get(&start, &end);

	PRINT_F(FORMAT, "average alloc and dealloc memory page",
		SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, (2 * NR_OF_MAP_RUNS)));
}
