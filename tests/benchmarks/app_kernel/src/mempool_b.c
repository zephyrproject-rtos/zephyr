/* mempool_b.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "master.h"

#ifdef MEMPOOL_BENCH

/**
 *
 * @brief Memory pool get/free test
 *
 * @return N/A
 */
void mempool_test(void)
{
	u32_t et; /* elapsed time */
	int i;
	s32_t return_value = 0;
	struct k_mem_block block;

	PRINT_STRING(dashline, output_file);
	et = BENCH_START();
	for (i = 0; i < NR_OF_POOL_RUNS; i++) {
		return_value |= k_mem_pool_alloc(&DEMOPOOL,
						&block,
						16,
						K_FOREVER);
		k_mem_pool_free(&block);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	if (return_value != 0) {
		k_panic();
	}
	PRINT_F(output_file, FORMAT,
		"average alloc and dealloc memory pool block",
		SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, (2 * NR_OF_POOL_RUNS)));
}

#endif /* MEMPOOL_BENCH */
