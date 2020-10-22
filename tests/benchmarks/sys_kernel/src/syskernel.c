/* syskernel.c */

/*
 * Copyright (c) 1997-2010, 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>

#include "syskernel.h"

#include <string.h>

K_THREAD_STACK_DEFINE(thread_stack1, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_stack2, STACK_SIZE);
struct k_thread thread_data1;
struct k_thread thread_data2;

char Msg[256];

FILE *output_file;

const char sz_success[] = "SUCCESSFUL";
const char sz_partial[] = "PARTIAL";
const char sz_fail[] = "FAILED";

/* time necessary to read the time */
uint32_t tm_off;

/* Holds the loop count that need to be carried out. */
uint32_t number_of_loops;

/**
 *
 * @brief Get the time ticks before test starts
 *
 * Routine does necessary preparations for the test to start
 *
 * @return N/A
 */
void begin_test(void)
{
	/*
	 * Invoke bench_test_start in order to be able to use
	 * timestamp_check static variable.
	 */
	bench_test_start();
}

/**
 *
 * @brief Checks number of tests and calculate average time
 *
 * @return 1 if success and 0 on failure
 *
 * @param i   Number of tests.
 * @param t   Time in ticks for the whole test.
 */
int check_result(int i, uint32_t t)
{
	/*
	 * bench_test_end checks timestamp_check static variable.
	 * bench_test_start modifies it
	 */
	if (bench_test_end() != 0) {
		fprintf(output_file, sz_case_result_fmt, sz_fail);
		fprintf(output_file, sz_case_details_fmt,
				"timer tick happened. Results are inaccurate");
		fprintf(output_file, sz_case_end_fmt);
		return 0;
	}
	if (i != number_of_loops) {
		fprintf(output_file, sz_case_result_fmt, sz_fail);
		fprintf(output_file, sz_case_details_fmt, "loop counter = ");
		fprintf(output_file, "%i !!!", i);
		fprintf(output_file, sz_case_end_fmt);
		return 0;
	}
	fprintf(output_file, sz_case_result_fmt, sz_success);
	fprintf(output_file, sz_case_details_fmt,
			"Average time for 1 iteration: ");
	fprintf(output_file, sz_case_timing_fmt,
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(t, number_of_loops));

	fprintf(output_file, sz_case_end_fmt);
	return 1;
}

/**
 *
 * @brief Prepares the test output
 *
 * @param continuously   Run test till the user presses the key.
 *
 * @return N/A
 */

void init_output(int *continuously)
{
	ARG_UNUSED(continuously);

	/*
	 * send all printf and fprintf to console
	 */
	output_file = stdout;
}


/**
 *
 * @brief Close output for the test
 *
 * @return N/A
 */
void output_close(void)
{
}

/**
 *
 * @brief Perform all selected benchmarks
 *
 * @return N/A
 */
void main(void)
{
	int	    continuously = 0;
	int	    test_result;

	number_of_loops = NUMBER_OF_LOOPS;

	/* The following code is needed to make the benchmakring run on
	 * slower platforms.
	 */
	uint64_t time_stamp = z_tick_get();

	k_sleep(K_MSEC(1));

	uint64_t time_stamp_2 = z_tick_get();

	if (time_stamp_2 - time_stamp > 1) {
		number_of_loops = 10U;
	}

	init_output(&continuously);
	bench_test_init();



	do {
		fprintf(output_file, sz_module_title_fmt,
			"kernel API test");
		fprintf(output_file, sz_kernel_ver_fmt,
			sys_kernel_version_get());
		fprintf(output_file,
			"\n\nEach test below is repeated %d times;\n"
			"average time for one iteration is displayed.",
			number_of_loops);

		test_result = 0;

		test_result += sema_test();
		test_result += lifo_test();
		test_result += fifo_test();
		test_result += stack_test();
		test_result += mem_slab_test();

		if (test_result) {
			/* sema/lifo/fifo/stack/mem_slab account for 14 tests in total */
			if (test_result == 14) {
				fprintf(output_file, sz_module_result_fmt,
					sz_success);
			} else {
				fprintf(output_file, sz_module_result_fmt,
					sz_partial);
			}
		} else {
			fprintf(output_file, sz_module_result_fmt, sz_fail);
		}
		TC_PRINT_RUNID;

	} while (continuously);

	output_close();
}
