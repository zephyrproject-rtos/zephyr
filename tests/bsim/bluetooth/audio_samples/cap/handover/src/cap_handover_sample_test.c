/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <inttypes.h>
#include <stdint.h>

#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "bs_utils.h"
#include "bstests.h"

#define WAIT_TIME 60 /* Seconds */

#define PASS_THRESHOLD 100 /* Audio packets */

extern enum bst_result_t bst_result;

#define FAIL(...)                                                                                  \
	do {                                                                                       \
		bst_result = Failed;                                                               \
		bs_trace_error_time_line(__VA_ARGS__);                                             \
	} while (0)

#define PASS(...)                                                                                  \
	do {                                                                                       \
		bst_result = Passed;                                                               \
		bs_trace_info_time(1, __VA_ARGS__);                                                \
	} while (0)

static void test_cap_handover_sample_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME * 1e6);
	bst_result = In_progress;
}

static void test_cap_handover_sample_tick(bs_time_t HW_device_time)
{
	ARG_UNUSED(HW_device_time);
	/*
	 * If in WAIT_TIME seconds we did not get enough packets through
	 * we consider the test failed
	 */

	extern uint64_t total_tx_iso_packet_count;

	bs_trace_info_time(2, "%" PRIu64 " ISO packets sent, expected >= %i\n",
			   total_tx_iso_packet_count, PASS_THRESHOLD);

	if (total_tx_iso_packet_count < PASS_THRESHOLD) {
		FAIL("cap_handover FAILED (Did not pass after %d seconds)\n", WAIT_TIME);
		return;
	}

	PASS("cap_handover PASSED\n");
}

static const struct bst_test_instance test_sample[] = {
	{
		.test_id = "cap_handover",
		.test_descr = "Test for the CAP handover samples. It runs the cap_handover sample "
			      "as a central with the cap_acceptor sample as the peripheral, and "
			      "verifies that at least PASS_THRESHOLD ISO packets are sent",
		.test_post_init_f = test_cap_handover_sample_init,
		.test_tick_f = test_cap_handover_sample_tick,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_cap_handover_sample_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_sample);
	return tests;
}
