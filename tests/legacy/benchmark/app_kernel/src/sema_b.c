/* sema_b.c */

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

#ifdef SEMA_BENCH


/**
 *
 * @brief Semaphore signal speed test
 *
 * @return N/A
 */
void sema_test(void)
{
	uint32_t et; /* elapsed Time */
	int i;

	PRINT_STRING(dashline, output_file);
	et = BENCH_START();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_give(SEM0);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "signal semaphore",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_SEMA_RUNS));

	task_sem_reset(SEM1);
	task_sem_give(STARTRCV);

	et = BENCH_START();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_give(SEM1);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "signal to waiting high pri task",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_SEMA_RUNS));

	et = BENCH_START();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_give(SEM1);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT,
			"signal to waiting high pri task, with timeout",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_SEMA_RUNS));

	et = BENCH_START();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_give(SEM2);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "signal to waitm (2)",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_SEMA_RUNS));

	et = BENCH_START();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_give(SEM2);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "signal to waitm (2), with timeout",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_SEMA_RUNS));

	et = BENCH_START();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_give(SEM3);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "signal to waitm (3)",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_SEMA_RUNS));

	et = BENCH_START();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_give(SEM3);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "signal to waitm (3), with timeout",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_SEMA_RUNS));

	et = BENCH_START();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_give(SEM4);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "signal to waitm (4)",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_SEMA_RUNS));

	et = BENCH_START();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_give(SEM4);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "signal to waitm (4), with timeout",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_SEMA_RUNS));
}

#endif /* SEMA_BENCH */
