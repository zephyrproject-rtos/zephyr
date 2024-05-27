/*
 * Copyright (c) 2023-2024 Nordic Semiconductor ASA
 * Copyright (c) 2017-2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_types.h"
#include "bs_tracing.h"
#include "bs_utils.h"
#include "time_machine.h"
#include "bstests.h"

#define WAIT_TIME 10 /* Seconds */

#define PASS_THRESHOLD 5 /* packets */

extern enum bst_result_t bst_result;

#define FAIL(...)					\
	do {						\
		bst_result = Failed;			\
		bs_trace_error_time_line(__VA_ARGS__);	\
	} while (0)

#define PASS(...)					\
	do {						\
		bst_result = Passed;			\
		bs_trace_info_time(1, __VA_ARGS__);	\
	} while (0)

static void test_sample_init(void)
{
	/* We set an absolute deadline in 30 seconds */
	bst_ticker_set_next_tick_absolute(WAIT_TIME*1e6);
	bst_result = In_progress;
}

static void test_sample_tick(bs_time_t HW_device_time)
{
	/*
	 * If in WAIT_TIME seconds we did not get enough packets through
	 * we consider the test failed
	 */

	extern uint64_t total_rx_count;

	bs_trace_info_time(2, "%"PRIu64" packets received, expected >= %i\n",
			   total_rx_count, PASS_THRESHOLD);

	if (total_rx_count >= PASS_THRESHOLD) {
		PASS("PASSED\n");
		bs_trace_exit("Done, disconnecting from simulation\n");
	} else {
		FAIL("FAILED (Did not pass after %i seconds)\n", WAIT_TIME);
	}
}

static const struct bst_test_instance test_sample[] = {
	{
		.test_id = "central_hr_peripheral_hr",
		.test_descr = "Test based on the peripheral and central HR samples. "
			      "It expects to be connected to a compatible sample, "
			      "waits for " STR(WAIT_TIME) " seconds, and checks how "
			      "many packets have been received correctly",
		.test_pre_init_f = test_sample_init,
		.test_tick_f = test_sample_tick,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_sample_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_sample);
	return tests;
}
