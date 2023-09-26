/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2017-2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_types.h"
#include "bs_tracing.h"
#include "bs_utils.h"
#include "time_machine.h"
#include "bstests.h"

#include "common.h" /* From echo_client */

#if defined(CONFIG_NET_L2_OPENTHREAD)
/* Open thread takes ~15 seconds to connect in ideal conditions */
#define WAIT_TIME 25 /* Seconds */
#else
#define WAIT_TIME 20 /* Seconds */
#endif

#define PASS_THRESHOLD 100 /* Packets */

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

static void test_echo_client_init(void)
{
	/* We set an absolute deadline in 30 seconds */
	bst_ticker_set_next_tick_absolute(WAIT_TIME*1e6);
	bst_result = In_progress;
}

static void test_echo_client_tick(bs_time_t HW_device_time)
{
	/*
	 * If in WAIT_TIME seconds we did not get enough packets through
	 * we consider the test failed
	 */

	extern struct configs conf;
	int packet_count = 0;

	if ((IS_ENABLED(CONFIG_NET_TCP)) && IS_ENABLED(CONFIG_NET_IPV6)) {
		packet_count = conf.ipv6.tcp.counter;
	} else if ((IS_ENABLED(CONFIG_NET_UDP)) && IS_ENABLED(CONFIG_NET_IPV6)) {
		packet_count = conf.ipv6.udp.counter;
	}

	bs_trace_info_time(2, "%i packets received, expected >= %i\n",
			   packet_count, PASS_THRESHOLD);

	if (packet_count >= PASS_THRESHOLD) {
		PASS("echo_client PASSED\n");
		bs_trace_exit("Done, disconnecting from simulation\n");
	} else {
		FAIL("echo_client FAILED (Did not pass after %i seconds)\n",
		     WAIT_TIME);
	}
}

static const struct bst_test_instance test_echo_client[] = {
	{
		.test_id = "echo_client",
		.test_descr = "Test based on the echo client sample. "
			      "It expects to be connected to a compatible echo server, "
			      "waits for " STR(WAIT_TIME) " seconds, and checks how "
			      "many packets have been exchanged correctly",
		.test_post_init_f = test_echo_client_init,
		.test_tick_f = test_echo_client_tick,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_echo_client_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_echo_client);
	return tests;
}
