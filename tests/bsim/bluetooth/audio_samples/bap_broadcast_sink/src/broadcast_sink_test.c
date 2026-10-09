/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2017-2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/autoconf.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "bs_utils.h"
#include "bstests.h"

#include "audio/bap_broadcast_sink/src/stream_rx.h"

BUILD_ASSERT(IS_ENABLED(CONFIG_LIBLC3) && CONFIG_INFO_REPORTING_INTERVAL > 0);

#define WAIT_TIME 120 /* Seconds */
#define RESYNC_WAIT_TIME_S 150
#define RESYNC_POLL_PERIOD_US 100000U

#define PASS_THRESHOLD 100 /* Audio packets */
#define RESYNC_DECODE_THRESHOLD 100U

extern enum bst_result_t bst_result;
extern struct stream_rx rx_streams[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];

static bool decoders_released;

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

static void test_broadcast_sink_sample_init(void)
{
	/* We set an absolute deadline in 30 seconds */
	bst_ticker_set_next_tick_absolute(WAIT_TIME*1e6);
	bst_result = In_progress;
}

static void test_broadcast_sink_sample_tick(bs_time_t HW_device_time)
{
	ARG_UNUSED(HW_device_time);

	/*
	 * If in WAIT_TIME seconds we did not get enough packets through
	 * we consider the test failed
	 */

	extern uint64_t total_rx_iso_packet_count;

	bs_trace_info_time(2, "%"PRIu64" packets received, expected >= %i\n",
			   total_rx_iso_packet_count, PASS_THRESHOLD);

	if (total_rx_iso_packet_count >= PASS_THRESHOLD) {
		PASS("broadcast_sink PASSED\n");
		bs_trace_exit("Done, disconnecting from simulation\n");
	} else {
		FAIL("broadcast_sink FAILED (Did not pass after %i seconds)\n",
		     WAIT_TIME);
	}
}

static void test_broadcast_sink_resync_init(void)
{
	bst_ticker_set_period(RESYNC_POLL_PERIOD_US);
	bst_result = In_progress;
}

static bool any_decoder(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(rx_streams); i++) {
		if (rx_streams[i].lc3_decoder != NULL) {
			return true;
		}
	}

	return false;
}

static size_t max_active_decoded_cnt(void)
{
	size_t max_cnt = 0U;

	for (size_t i = 0U; i < ARRAY_SIZE(rx_streams); i++) {
		if (rx_streams[i].lc3_decoder != NULL) {
			max_cnt = MAX(max_cnt, rx_streams[i].reporting_info.lc3_decoded_cnt);
		}
	}

	return max_cnt;
}

static void test_broadcast_sink_resync_tick(bs_time_t HW_device_time)
{
	extern uint64_t total_rx_iso_packet_count;

	if (!decoders_released) {
		if (total_rx_iso_packet_count >= PASS_THRESHOLD && !any_decoder()) {
			bs_trace_info_time(2, "%"PRIu64" packets received, decoders released\n",
					   total_rx_iso_packet_count);
			decoders_released = true;
		}
	} else if (max_active_decoded_cnt() >= RESYNC_DECODE_THRESHOLD) {
		PASS("broadcast_sink_resync PASSED\n");
		bs_trace_exit("Done, disconnecting from simulation\n");
	}

	if (bst_result == In_progress && HW_device_time >= RESYNC_WAIT_TIME_S * 1e6) {
		FAIL("broadcast_sink_resync FAILED (Did not pass after %i seconds: %"PRIu64
		     " packets, decoders released %d, %zu SDUs decoded)\n",
		     RESYNC_WAIT_TIME_S, total_rx_iso_packet_count, decoders_released,
		     max_active_decoded_cnt());
	}
}

static const struct bst_test_instance test_sample[] = {
	{
		.test_id = "bap_broadcast_sink",
		.test_descr = "Test based on the broadcast audio sink sample. "
			      "It expects to be connected to a compatible broadcast audio source, "
			      "waits for " STR(WAIT_TIME) " seconds, and checks how "
			      "many ISO packets have been received correctly",
		.test_pre_init_f = test_broadcast_sink_sample_init,
		.test_tick_f = test_broadcast_sink_sample_tick,
	},
	{
		.test_id = "bap_broadcast_sink_resync",
		.test_descr = "Test based on the broadcast audio sink sample. "
			      "It expects to be connected to a compatible broadcast audio source "
			      "that stops and restarts, and checks that the LC3 decoders are "
			      "released at the stop and that decoding resumes after the re-sync, "
			      "within " STR(RESYNC_WAIT_TIME_S) " seconds",
		.test_pre_init_f = test_broadcast_sink_resync_init,
		.test_tick_f = test_broadcast_sink_resync_tick,
	},
	BSTEST_END_MARKER};

struct bst_test_list *test_broadcast_sink_test_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_sample);
	return tests;
}
