/* mempool_b.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
	uint32_t et; /* elapsed time */
	int i;
	struct k_block block;

	PRINT_STRING(dashline, output_file);
	et = BENCH_START();
	for (i = 0; i < NR_OF_POOL_RUNS; i++) {
		task_mem_pool_alloc(&block, DEMOPOOL, 16, TICKS_UNLIMITED);
		task_mem_pool_free(&block);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT,
			"average alloc and dealloc memory pool block",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, (2 * NR_OF_POOL_RUNS)));
}

#endif /* MEMPOOL_BENCH */
