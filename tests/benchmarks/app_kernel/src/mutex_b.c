/* mutex_b.c */

/*
 * Copyright (c) 1997-2010, 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "master.h"

/**
 * @brief Mutex lock/unlock test
 */
void mutex_test(void)
{
	uint64_t et; /* elapsed time */
	int i;
	timing_t  start;
	timing_t  end;

	PRINT_STRING(dashline);
	start = timing_timestamp_get();
	for (i = 0; i < NR_OF_MUTEX_RUNS; i++) {
		k_mutex_lock(&DEMO_MUTEX, K_FOREVER);
		k_mutex_unlock(&DEMO_MUTEX);
	}
	end = timing_timestamp_get();
	et = timing_cycles_get(&start, &end);

	PRINT_F(FORMAT, "average lock and unlock mutex",
		timing_cycles_to_ns_avg(et, (2 * NR_OF_MUTEX_RUNS)));
}
