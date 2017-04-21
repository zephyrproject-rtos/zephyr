/* sema_b.c */

/*
 * Copyright (c) 1997-2010, 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
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
	u32_t et; /* elapsed Time */
	int i;

	PRINT_STRING(dashline, output_file);
	et = BENCH_START();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
	  k_sem_give(&SEM0);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "signal semaphore",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_SEMA_RUNS));

	k_sem_reset(&SEM1);
	k_sem_give(&STARTRCV);

	et = BENCH_START();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		k_sem_give(&SEM1);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT, "signal to waiting high pri task",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_SEMA_RUNS));

	et = BENCH_START();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		k_sem_give(&SEM1);
	}
	et = TIME_STAMP_DELTA_GET(et);
	check_result();

	PRINT_F(output_file, FORMAT,
			"signal to waiting high pri task, with timeout",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(et, NR_OF_SEMA_RUNS));

}

#endif /* SEMA_BENCH */
