/* msgq_b.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "master.h"

/**
 * @brief Message queue transfer speed test
 */
void message_queue_test(void)
{
	uint64_t et; /* elapsed time */
	int i;
	timing_t  start;
	timing_t  end;

	PRINT_STRING(dashline);
	start = timing_timestamp_get();
	for (i = 0; i < NR_OF_MSGQ_RUNS; i++) {
		k_msgq_put(&DEMOQX1, data_bench, K_FOREVER);
	}
	end = timing_timestamp_get();
	et = timing_cycles_get(&start, &end);
	PRINT_F(FORMAT, "enqueue 1 byte msg in MSGQ",
		timing_cycles_to_ns_avg(et, NR_OF_MSGQ_RUNS));

	start = timing_timestamp_get();
	for (i = 0; i < NR_OF_MSGQ_RUNS; i++) {
		k_msgq_get(&DEMOQX1, data_bench, K_FOREVER);
	}
	end = timing_timestamp_get();
	et = timing_cycles_get(&start, &end);

	PRINT_F(FORMAT, "dequeue 1 byte msg from MSGQ",
		timing_cycles_to_ns_avg(et, NR_OF_MSGQ_RUNS));

	start = timing_timestamp_get();
	for (i = 0; i < NR_OF_MSGQ_RUNS; i++) {
		k_msgq_put(&DEMOQX4, data_bench, K_FOREVER);
	}
	end = timing_timestamp_get();
	et = timing_cycles_get(&start, &end);

	PRINT_F(FORMAT, "enqueue 4 bytes msg in MSGQ",
		timing_cycles_to_ns_avg(et, NR_OF_MSGQ_RUNS));

	start = timing_timestamp_get();
	for (i = 0; i < NR_OF_MSGQ_RUNS; i++) {
		k_msgq_get(&DEMOQX4, data_bench, K_FOREVER);
	}
	end = timing_timestamp_get();
	et = timing_cycles_get(&start, &end);

	PRINT_F(FORMAT, "dequeue 4 bytes msg in MSGQ",
		timing_cycles_to_ns_avg(et, NR_OF_MSGQ_RUNS));

	start = timing_timestamp_get();
	for (i = 0; i < NR_OF_MSGQ_RUNS; i++) {
		k_msgq_put(&DEMOQX192, data_bench, K_FOREVER);
	}
	end = timing_timestamp_get();
	et = timing_cycles_get(&start, &end);

	PRINT_F(FORMAT, "enqueue 192 bytes msg in MSGQ",
		timing_cycles_to_ns_avg(et, NR_OF_MSGQ_RUNS));

	start = timing_timestamp_get();
	for (i = 0; i < NR_OF_MSGQ_RUNS; i++) {
		k_msgq_get(&DEMOQX192, data_bench, K_FOREVER);
	}
	end = timing_timestamp_get();
	et = timing_cycles_get(&start, &end);

	PRINT_F(FORMAT, "dequeue 192 bytes msg in MSGQ",
		timing_cycles_to_ns_avg(et, NR_OF_MSGQ_RUNS));

	k_sem_give(&STARTRCV);

	start = timing_timestamp_get();
	for (i = 0; i < NR_OF_MSGQ_RUNS; i++) {
		k_msgq_put(&DEMOQX1, data_bench, K_FOREVER);
	}
	end = timing_timestamp_get();
	et = timing_cycles_get(&start, &end);

	PRINT_F(FORMAT,
		"enqueue 1 byte msg in MSGQ to a waiting higher priority task",
		timing_cycles_to_ns_avg(et, NR_OF_MSGQ_RUNS));

	start = timing_timestamp_get();
	for (i = 0; i < NR_OF_MSGQ_RUNS; i++) {
		k_msgq_put(&DEMOQX4, data_bench, K_FOREVER);
	}
	end = timing_timestamp_get();
	et = timing_cycles_get(&start, &end);

	PRINT_F(FORMAT,
		"enqueue 4 bytes in MSGQ to a waiting higher priority task",
		timing_cycles_to_ns_avg(et, NR_OF_MSGQ_RUNS));

	start = timing_timestamp_get();
	for (i = 0; i < NR_OF_MSGQ_RUNS; i++) {
		k_msgq_put(&DEMOQX192, data_bench, K_FOREVER);
	}
	end = timing_timestamp_get();
	et = timing_cycles_get(&start, &end);

	PRINT_F(FORMAT,
		"enqueue 192 bytes in MSGQ to a waiting higher priority task",
		timing_cycles_to_ns_avg(et, NR_OF_MSGQ_RUNS));
}
