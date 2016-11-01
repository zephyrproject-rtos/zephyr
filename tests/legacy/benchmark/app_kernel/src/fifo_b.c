/* fifo_b.c */

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

#ifdef FIFO_BENCH

/**
 *
 * @brief Queue transfer speed test
 *
 * @return N/A
 */
void queue_test(void)
{
	uint32_t et; /* elapsed time */
	int i;

	PRINT_STRING(dashline, output_file);
	et = BENCH_START();
	for (i = 0; i < NR_OF_FIFO_RUNS; i++) {
		task_fifo_put(DEMOQX1, data_bench, TICKS_UNLIMITED);
	}
	et = TIME_STAMP_DELTA_GET(et);

	PRINT_F(output_file, FORMAT, "enqueue 1 byte msg in FIFO",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_FIFO_RUNS));

	et = BENCH_START();
	for (i = 0; i < NR_OF_FIFO_RUNS; i++) {
		task_fifo_get(DEMOQX1, data_bench, TICKS_UNLIMITED);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "dequeue 1 byte msg in FIFO",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_FIFO_RUNS));

	et = BENCH_START();
	for (i = 0; i < NR_OF_FIFO_RUNS; i++) {
		task_fifo_put(DEMOQX4, data_bench, TICKS_UNLIMITED);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "enqueue 4 bytes msg in FIFO",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_FIFO_RUNS));

	et = BENCH_START();
	for (i = 0; i < NR_OF_FIFO_RUNS; i++) {
		task_fifo_get(DEMOQX4, data_bench, TICKS_UNLIMITED);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "dequeue 4 bytes msg in FIFO",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_FIFO_RUNS));

	task_sem_give(STARTRCV);

	et = BENCH_START();
	for (i = 0; i < NR_OF_FIFO_RUNS; i++) {
		task_fifo_put(DEMOQX1, data_bench, TICKS_UNLIMITED);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT,
			"enqueue 1 byte msg in FIFO to a waiting higher priority task",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_FIFO_RUNS));

	et = BENCH_START();
	for (i = 0; i < NR_OF_FIFO_RUNS; i++) {
		task_fifo_put(DEMOQX4, data_bench, TICKS_UNLIMITED);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT,
			"enqueue 4 bytes in FIFO to a waiting higher priority task",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_FIFO_RUNS));
}

#endif /* FIFO_BENCH */
