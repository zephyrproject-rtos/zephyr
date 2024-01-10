/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BUILD_DATE_TIME_TEST)

#include <stdlib.h>
#include <zephyr/ztest.h>
#include <zephyr/net/buf.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/transport/smp_dummy.h>
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <os_mgmt_processor.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <version.h>
#include <smp_internal.h>
#include "smp_test_util.h"

#define SMP_RESPONSE_WAIT_TIME 3
#define ZCBOR_BUFFER_SIZE 256
#define OUTPUT_BUFFER_SIZE 256
#define ZCBOR_HISTORY_ARRAY_SIZE 4

static struct net_buf *nb;

/* Responses to commands */
extern uint8_t *test_date_time;
const uint8_t response_all_board_revision_left[] = "Zephyr unknown " STRINGIFY(BUILD_VERSION) " "
						   KERNEL_VERSION_STRING " ";
const uint8_t response_all_board_revision_right[] = " " CONFIG_ARCH " " PROCESSOR_NAME " "
						   CONFIG_BOARD "@" CONFIG_BOARD_REVISION
						   " Zephyr";
const uint8_t response_all_left[] = "Zephyr unknown " STRINGIFY(BUILD_VERSION) " "
				    KERNEL_VERSION_STRING " ";
const uint8_t response_all_right[] = " " CONFIG_ARCH " " PROCESSOR_NAME " " CONFIG_BOARD " Zephyr";

const uint8_t query_build_date[] = "b";
const uint8_t query_all[] = "a";

#define DATE_CHECK_LEFT_CHARS 11
#define DATE_CHECK_RIGHT_CHARS 5
#define TIME_CHECK_HH_START_CHAR 11

#define TIME_HH_OFFSET 0
#define TIME_MM_OFFSET 3
#define TIME_SS_OFFSET 6

#define SECONDS_PER_HOUR 3600
#define SECONDS_PER_MINUTE 60

#define TIME_DIFFERENCE_ALLOWANCE 60

static int32_t time_string_to_seconds(const uint8_t *time_string)
{
	uint8_t time_hh;
	uint8_t time_mm;
	uint8_t time_ss;

	/* Convert times to separate fields and then to timestamps which can be compared */
	time_hh = ((time_string[TIME_HH_OFFSET] - '0') * 10) +
		  (time_string[TIME_HH_OFFSET + 1] - '0');
	time_mm = ((time_string[TIME_MM_OFFSET] - '0') * 10) +
		  (time_string[TIME_MM_OFFSET + 1] - '0');
	time_ss = ((time_string[TIME_SS_OFFSET] - '0') * 10) +
		  (time_string[TIME_SS_OFFSET + 1] - '0');

	return (time_hh * SECONDS_PER_HOUR) + (time_mm * SECONDS_PER_MINUTE) + time_ss;
}

ZTEST(os_mgmt_info_build_date, test_info_build_date_1_build_date)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct zcbor_string output = { 0 };
	size_t decoded = 0;
	int32_t expected_time_seconds;
	int32_t received_time_seconds;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, query_build_date, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send test echo command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element\n");

	zassert_equal(strlen(test_date_time), output.len,
		      "Expected to receive %d bytes but got %d\n",
		      strlen(test_date_time), output.len);

	/* Check left and right sides of date which should match */
	zassert_mem_equal(test_date_time, output.value, DATE_CHECK_LEFT_CHARS,
			  "Expected received data mismatch");
	zassert_mem_equal(&test_date_time[(strlen(test_date_time) - DATE_CHECK_RIGHT_CHARS)],
			  &output.value[(strlen(test_date_time) - DATE_CHECK_RIGHT_CHARS)],
			  DATE_CHECK_RIGHT_CHARS, "Expected received data mismatch");

	/* Extract time strings into timestamps */
	expected_time_seconds = time_string_to_seconds(&test_date_time[TIME_CHECK_HH_START_CHAR]);
	received_time_seconds = time_string_to_seconds(&output.value[TIME_CHECK_HH_START_CHAR]);

	zassert_within(expected_time_seconds, received_time_seconds, TIME_DIFFERENCE_ALLOWANCE,
		       "Expected times to be within %d seconds but got %d",
		       TIME_DIFFERENCE_ALLOWANCE,
		       abs(expected_time_seconds - received_time_seconds));
}

ZTEST(os_mgmt_info_build_date, test_info_build_date_2_all)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct zcbor_string output = { 0 };
	size_t decoded = 0;
	int32_t expected_time_seconds;
	int32_t received_time_seconds;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, query_all, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send test echo command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element\n");

	if (sizeof(CONFIG_BOARD_REVISION) > 1) {
		/* Check with board revision */
		zassert_equal((strlen(test_date_time) + strlen(response_all_board_revision_left) +
			       strlen(response_all_board_revision_right)), output.len,
			      "Expected to receive %d bytes but got %d\n",
			      (strlen(test_date_time) + strlen(response_all_board_revision_left) +
			       strlen(response_all_board_revision_right)), output.len);

		zassert_mem_equal(response_all_board_revision_left, output.value,
				  strlen(response_all_board_revision_left),
				  "Expected received data mismatch");
		zassert_mem_equal(response_all_board_revision_right,
				  &output.value[strlen(response_all_board_revision_left) +
				  strlen(test_date_time)],
				  strlen(response_all_board_revision_right),
				  "Expected received data mismatch");
	} else {
		/* Check without board revision */
		zassert_equal((strlen(test_date_time) + strlen(response_all_left) +
			       strlen(response_all_right)), output.len,
			      "Expected to receive %d bytes but got %d\n",
			      (strlen(test_date_time) + strlen(response_all_left) +
			       strlen(response_all_right)), output.len);

		zassert_mem_equal(response_all_left, output.value, strlen(response_all_left),
				  "Expected received data mismatch");
		zassert_mem_equal(response_all_right, &output.value[strlen(response_all_left) +
				  strlen(test_date_time)], strlen(response_all_right),
				  "Expected received data mismatch");
	}

	/* Extract time strings into timestamps */
	expected_time_seconds = time_string_to_seconds(&test_date_time[TIME_CHECK_HH_START_CHAR]);
	received_time_seconds = time_string_to_seconds(&output.value[(strlen(response_all_left) +
				TIME_CHECK_HH_START_CHAR)]);

	zassert_within(expected_time_seconds, received_time_seconds, TIME_DIFFERENCE_ALLOWANCE,
		       "Expected times to be within %d seconds but got %d",
		       TIME_DIFFERENCE_ALLOWANCE,
		       abs(expected_time_seconds - received_time_seconds));
}

static void cleanup_test(void *p)
{
	if (nb != NULL) {
		net_buf_unref(nb);
		nb = NULL;
	}
}

/* Build date/time test set */
ZTEST_SUITE(os_mgmt_info_build_date, NULL, NULL, NULL, cleanup_test, NULL);

#endif
