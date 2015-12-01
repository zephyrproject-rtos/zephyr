/* mutex_b.c */

/*
 * Copyright (c) 1997-2010, 2013-2015 Wind River Systems, Inc.
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

#ifdef MUTEX_BENCH

/**
 *
 * @brief Mutex lock/unlock test
 *
 * @return N/A
 */
void mutex_test(void)
{
	uint32_t et; /* elapsed time */
	int i;

	PRINT_STRING(dashline, output_file);
	et = BENCH_START();
	for (i = 0; i < NR_OF_MUTEX_RUNS; i++) {
		task_mutex_lock(DEMO_MUTEX, TICKS_UNLIMITED);
		task_mutex_unlock(DEMO_MUTEX);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "average lock and unlock mutex",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, (2 * NR_OF_MUTEX_RUNS)));
}

#endif /* MUTEX_BENCH */
