/*
 * Copyright (c) 2025 BayLibre SAS
 *   *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "syskernel.h"

static void *malloc_array[NUMBER_OF_LOOPS];

static void do_malloc(int loops)
{
	int i;

	for (i = 0; i < loops; i++) {
		malloc_array[i] = k_malloc(1);
	}
}

static void do_free(int loops)
{
	int i;

	for (i = 0; i < loops; i++) {
		k_free(malloc_array[i]);
	}
}

int malloc_test(void)
{
	uint32_t t;
	int return_value = 0;

	fprintf(output_file, sz_test_case_fmt,
		"malloc #1");
	fprintf(output_file, sz_description,
		"\n\tk_malloc");
	printf(sz_test_start_fmt);

	t = BENCH_START();
	do_malloc(number_of_loops);
	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(number_of_loops, t);

	fprintf(output_file, sz_test_case_fmt,
		"malloc #2");
	fprintf(output_file, sz_description,
		"\n\tk_free");
	printf(sz_test_start_fmt);

	t = BENCH_START();
	do_free(number_of_loops);
	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(number_of_loops, t);

	return return_value;
}
