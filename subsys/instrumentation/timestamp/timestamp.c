/*
 * Copyright 2023 Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/instrumentation/instrumentation.h>
#include <instr_timestamp.h>

__no_instrumentation__
int instr_timestamp_init(void)
{
	timing_init();
	timing_start();

	return 0;
}

__no_instrumentation__
uint64_t instr_timestamp_ns(void)
{
	timing_t bigbang = 0;
	timing_t now;
	uint64_t now_cycles;

	now = timing_counter_get();
	now_cycles = timing_cycles_get(&bigbang, &now);

	return timing_cycles_to_ns(now_cycles);
}
