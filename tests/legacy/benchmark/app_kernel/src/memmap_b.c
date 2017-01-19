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
	uint32_t et; /* elapsed time */
	int i;
	void* p;

	PRINT_STRING(dashline, output_file);
	et = BENCH_START();
	for (i = 0; i < NR_OF_MAP_RUNS; i++) {
		task_mem_map_alloc(MAP1, &p, TICKS_UNLIMITED);
		task_mem_map_free(MAP1, &p);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "average alloc and dealloc memory page",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, (2 * NR_OF_MAP_RUNS)));
}

#endif /* MEMMAP_BENCH */
