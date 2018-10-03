/* memmap_b.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "master.h"
#include <zephyr.h>

#ifdef MEMMAP_BENCH


/**
 *
 * @brief Memory map get/free test
 *
 * @return N/A
 */
void memorymap_test(void)
{
	u32_t et; /* elapsed time */
	int i;
	void *p;
	int alloc_status;

	PRINT_STRING(dashline, output_file);
	et = BENCH_START();
	for (i = 0; i < NR_OF_MAP_RUNS; i++) {
		alloc_status = k_mem_slab_alloc(&MAP1, &p, K_FOREVER);
		if (alloc_status != 0) {
			PRINT_F(output_file, FORMAT,
				"Error: Slab allocation failed.", alloc_status);
			break;
		}
		k_mem_slab_free(&MAP1, &p);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "average alloc and dealloc memory page",
		SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, (2 * NR_OF_MAP_RUNS)));
}

#endif /* MEMMAP_BENCH */
