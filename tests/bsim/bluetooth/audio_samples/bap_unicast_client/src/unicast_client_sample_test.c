/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2017-2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <inttypes.h>
#include <stdint.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "bs_utils.h"
#include "bstests.h"

#define WAIT_TIME 10 /* Seconds */

#define PASS_THRESHOLD 100 /* Audio packets */

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

static void test_unicast_client_sample_init(void)
{
	/* We set an absolute deadline in 30 seconds */
	bst_ticker_set_next_tick_absolute(WAIT_TIME*1e6);
	bst_result = In_progress;
}

static void test_unicast_client_sample_tick(bs_time_t HW_device_time)
{
	/*
	 * If in WAIT_TIME seconds we did not get enough packets through
	 * we consider the test failed
	 */

	extern uint64_t unicast_audio_recv_ctr;

	bs_trace_info_time(2, "%"PRIu64" packets received, expected >= %i\n",
			   unicast_audio_recv_ctr, PASS_THRESHOLD);

	if (unicast_audio_recv_ctr >= PASS_THRESHOLD) {
		PASS("unicast_client PASSED\n");
		bs_trace_exit("Done, disconnecting from simulation\n");
	} else {
		FAIL("unicast_client FAILED (Did not pass after %i seconds)\n",
		     WAIT_TIME);
	}
}

static const struct bst_test_instance test_sample[] = {
	{
		.test_id = "unicast_client",
		.test_descr = "Test based on the unicast client sample. "
			      "It expects to be connected to a compatible unicast server, "
			      "waits for " STR(WAIT_TIME) " seconds, and checks how "
			      "many audio packets have been received correctly",
		.test_pre_init_f = test_unicast_client_sample_init,
		.test_tick_f = test_unicast_client_sample_tick,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_unicast_client_sample_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_sample);
	return tests;
}
