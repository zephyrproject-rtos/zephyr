/*
 * Copyright (c) 2023-2024 Nordic Semiconductor ASA
 * Copyright (c) 2017-2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <inttypes.h>
#include <stdint.h>

#include <zephyr/sys/util_macro.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "bs_utils.h"
#include "bstests.h"

#define WAIT_TIME 10 /* Seconds */

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

static void test_cap_acceptor_sample_init(void)
{
	/* We set an absolute deadline in 30 seconds */
	bst_ticker_set_next_tick_absolute(WAIT_TIME * 1e6);
	bst_result = In_progress;
}

static void test_cap_acceptor_sample_tick(bs_time_t HW_device_time)
{
	/*
	 * If in WAIT_TIME seconds we did not get enough packets through
	 * we consider the test failed
	 */

	if (IS_ENABLED(CONFIG_SAMPLE_UNICAST)) {
		extern uint64_t total_unicast_rx_iso_packet_count;
		extern uint64_t total_unicast_tx_iso_packet_count;

		bs_trace_info_time(2, "%" PRIu64 " unicast packets received, expected >= %i\n",
				   total_unicast_tx_iso_packet_count, PASS_THRESHOLD);
		bs_trace_info_time(2, "%" PRIu64 " unicast packets sent, expected >= %i\n",
				   total_unicast_tx_iso_packet_count, PASS_THRESHOLD);

		if (total_unicast_rx_iso_packet_count < PASS_THRESHOLD ||
		    total_unicast_tx_iso_packet_count < PASS_THRESHOLD) {
			FAIL("cap_acceptor FAILED with(Did not pass after %d seconds)\n ",
			     WAIT_TIME);
			return;
		}
	}

	if (IS_ENABLED(CONFIG_SAMPLE_BROADCAST)) {
		extern uint64_t total_broadcast_rx_iso_packet_count;

		bs_trace_info_time(2, "%" PRIu64 " broadcast packets received, expected >= %i\n",
				   total_broadcast_rx_iso_packet_count, PASS_THRESHOLD);

		if (total_broadcast_rx_iso_packet_count < PASS_THRESHOLD) {
			FAIL("cap_acceptor FAILED with (Did not pass after %d seconds)\n ",
			     WAIT_TIME);
			return;
		}
	}

	PASS("cap_acceptor PASSED\n");
}

static const struct bst_test_instance test_sample[] = {
	{
		.test_id = "cap_acceptor",
		.test_descr = "Test based on the unicast client sample. "
			      "It expects to be connected to a compatible unicast server, "
			      "waits for " STR(WAIT_TIME) " seconds, and checks how "
			      "many audio packets have been received correctly",
		.test_post_init_f = test_cap_acceptor_sample_init,
		.test_tick_f = test_cap_acceptor_sample_tick,
	},
	BSTEST_END_MARKER
};

/* TODO: Add test of reconnection */

struct bst_test_list *test_cap_acceptor_sample_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_sample);
	return tests;
}
