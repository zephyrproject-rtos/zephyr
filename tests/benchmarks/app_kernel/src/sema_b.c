/* sema_b.c */

/*
 * Copyright (c) 1997-2010, 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "master.h"

/**
 * @brief Semaphore signal speed test
 */
void sema_test(void)
{
	uint64_t et; /* elapsed Time */
	int i;
	timing_t  start;
	timing_t  end;

	PRINT_STRING(dashline);
	start = timing_timestamp_get();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
	  k_sem_give(&SEM0);
	}
	end = timing_timestamp_get();
	et = timing_cycles_get(&start, &end);

	PRINT_F(FORMAT, "signal semaphore",
		timing_cycles_to_ns_avg(et, NR_OF_SEMA_RUNS));

	k_sem_reset(&SEM1);
	k_sem_give(&STARTRCV);

	start = timing_timestamp_get();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		k_sem_give(&SEM1);
	}
	end = timing_timestamp_get();
	et = timing_cycles_get(&start, &end);

	PRINT_F(FORMAT, "signal to waiting high pri task",
		timing_cycles_to_ns_avg(et, NR_OF_SEMA_RUNS));

	start = timing_timestamp_get();
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		k_sem_give(&SEM1);
	}
	end = timing_timestamp_get();
	et = timing_cycles_get(&start, &end);

	PRINT_F(FORMAT, "signal to waiting high pri task, with timeout",
		timing_cycles_to_ns_avg(et, NR_OF_SEMA_RUNS));
}
