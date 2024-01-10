/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2023 Jamie M.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net/buf.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/transport/smp_dummy.h>
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <string.h>
#include <smp_internal.h>
#include <zephyr/drivers/rtc.h>
#include "smp_test_util.h"

#define SMP_RESPONSE_WAIT_TIME 3
#define ZCBOR_BUFFER_SIZE 256
#define OUTPUT_BUFFER_SIZE 256
#define ZCBOR_HISTORY_ARRAY_SIZE 4

/* Test sets */
enum {
	OS_MGMT_DATETIME_TEST_SET_TIME_NOT_SET = 0,
	OS_MGMT_DATETIME_TEST_SET_TIME_SET,
#ifdef CONFIG_MCUMGR_GRP_OS_DATETIME_HOOK
	OS_MGMT_DATETIME_TEST_SET_HOOKS,
#endif

	OS_MGMT_DATETIME_TEST_SET_COUNT
};

static struct net_buf *nb;

struct state {
	uint8_t test_set;
};

static struct state test_state = {
	.test_set = 0,
};

static struct rtc_time valid_time = {
	.tm_sec = 13,
	.tm_min = 40,
	.tm_hour = 4,
	.tm_mday = 4,
	.tm_mon = 8,
	.tm_year = 2023,
};

static struct rtc_time valid_time2 = {
	.tm_sec = 5,
	.tm_min = 4,
	.tm_hour = 3,
	.tm_mday = 2,
	.tm_mon = 1,
	.tm_year = 2001,
};

static const char valid_time_string[] = "2023-08-04T04:40:13";
static const char valid_time2_string[] = "2001-01-02T03:04:05";
static const char invalid_time_string[] = "abcdefghij";
static const char invalid_time2_string[] = "20a1-b1-aTbb:dd:qq";
static const char invalid_time3_string[] = "1820-01-02T03:04:05";

struct group_error {
	uint16_t group;
	uint16_t rc;
	bool found;
};

#ifdef CONFIG_MCUMGR_GRP_OS_DATETIME_HOOK
static bool hook_get_ran;
static bool hook_set_ran;
static bool hook_other_ran;
static uint8_t hook_set_data_size;
static uint8_t hook_set_data[64];
#endif

static void cleanup_test(void *p);

static bool mcumgr_ret_decode(zcbor_state_t *state, struct group_error *result)
{
	bool ok;
	size_t decoded;
	uint32_t tmp_group;
	uint32_t tmp_rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("group", zcbor_uint32_decode, &tmp_group),
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_uint32_decode, &tmp_rc),
	};

	result->found = false;

	ok = zcbor_map_decode_bulk(state, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	if (ok &&
	    zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode), "group") &&
	    zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode), "rc")) {
		result->group = (uint16_t)tmp_group;
		result->rc = (uint16_t)tmp_rc;
		result->found = true;
	}

	return ok;
}

#ifdef CONFIG_MCUMGR_GRP_OS_DATETIME_HOOK
static enum mgmt_cb_return os_mgmt_datetime_callback(uint32_t event,
						     enum mgmt_cb_return prev_status, int32_t *rc,
						     uint16_t *group, bool *abort_more,
						     void *data, size_t data_size)
{
	if (event == MGMT_EVT_OP_OS_MGMT_DATETIME_GET) {
		hook_get_ran = true;

		*rc = MGMT_ERR_EBUSY;
		return MGMT_CB_ERROR_RC;
	} else if (event == MGMT_EVT_OP_OS_MGMT_DATETIME_SET) {
		hook_set_ran = true;
		hook_set_data_size = data_size;
		memcpy(hook_set_data, data, data_size);

		*rc = MGMT_ERR_EACCESSDENIED;
		return MGMT_CB_ERROR_RC;
	}

	hook_other_ran = true;
	return MGMT_CB_OK;
}

static struct mgmt_callback os_datetime_callbacks = {
	.callback = os_mgmt_datetime_callback,
	.event_id = (MGMT_EVT_OP_OS_MGMT_DATETIME_GET | MGMT_EVT_OP_OS_MGMT_DATETIME_SET),
};
#endif

ZTEST(os_mgmt_datetime_not_set, test_datetime_get_not_set_v1)
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
	struct group_error group_error;
	int rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("datetime", zcbor_tstr_decode, &output),
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &rc),
		ZCBOR_MAP_DECODE_KEY_DECODER("err", mcumgr_ret_decode, &group_error),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_get_packet(zse, false, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "datetime"), "Did not expect to receive datetime element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		     "rc"), "Did not expect to receive rc element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "err"), "Expected to receive err element");
	zassert_equal(rc, MGMT_ERR_ENOENT, "Expected 'rc' to be no entity");
}

ZTEST(os_mgmt_datetime_not_set, test_datetime_get_not_set_v2)
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
	struct group_error group_error;
	int rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("datetime", zcbor_tstr_decode, &output),
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &rc),
		ZCBOR_MAP_DECODE_KEY_DECODER("err", mcumgr_ret_decode, &group_error),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_get_packet(zse, true, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "datetime"), "Did not expect to receive datetime element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "rc"), "Did not expect to receive rc element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		     "err"), "Expected to receive err element");
	zassert_equal(group_error.group, MGMT_GROUP_ID_OS, "Expected 'err' -> 'group' to be OS");
	zassert_equal(group_error.rc, OS_MGMT_ERR_RTC_NOT_SET,
		      "Expected 'err' -> 'rc' to be RTC not set");
}

ZTEST(os_mgmt_datetime_not_set, test_datetime_set_invalid_v1_1)
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
	struct group_error group_error;
	int rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("datetime", zcbor_tstr_decode, &output),
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &rc),
		ZCBOR_MAP_DECODE_KEY_DECODER("err", mcumgr_ret_decode, &group_error),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 4, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_set_packet_str(zse, false, invalid_time_string, buffer,
						   buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "datetime"), "Did not expect to receive datetime element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		     "rc"), "Expected to receive rc element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "err"), "Did not expect to receive err element");
	zassert_equal(rc, MGMT_ERR_EINVAL, "Expected 'rc' to be invalid value");

	/* Clean up test */
	cleanup_test(NULL);
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;

	/* Query time and ensure it is not set */
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_get_packet(zse, false, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "datetime"), "Did not expect to receive datetime element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		     "rc"), "Did not expect to receive rc element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "err"), "Expected to receive err element");
	zassert_equal(rc, MGMT_ERR_ENOENT, "Expected 'rc' to be no entity");
}

ZTEST(os_mgmt_datetime_not_set, test_datetime_set_invalid_v1_2)
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
	struct group_error group_error;
	int rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("datetime", zcbor_tstr_decode, &output),
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &rc),
		ZCBOR_MAP_DECODE_KEY_DECODER("err", mcumgr_ret_decode, &group_error),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 4, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_set_packet_str(zse, false, invalid_time2_string, buffer,
						   buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "datetime"), "Did not expect to receive datetime element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		     "rc"), "Expected to receive rc element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "err"), "Did not expect to receive err element");
	zassert_equal(rc, MGMT_ERR_EINVAL, "Expected 'rc' to be invalid value");

	/* Clean up test */
	cleanup_test(NULL);
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;

	/* Query time and ensure it is not set */
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_get_packet(zse, false, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "datetime"), "Did not expect to receive datetime element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		     "rc"), "Did not expect to receive rc element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "err"), "Expected to receive err element");
	zassert_equal(rc, MGMT_ERR_ENOENT, "Expected 'rc' to be no entity");
}

ZTEST(os_mgmt_datetime_not_set, test_datetime_set_invalid_v1_3)
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
	struct group_error group_error;
	int rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("datetime", zcbor_tstr_decode, &output),
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &rc),
		ZCBOR_MAP_DECODE_KEY_DECODER("err", mcumgr_ret_decode, &group_error),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 4, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_set_packet_str(zse, false, invalid_time3_string, buffer,
						   buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "datetime"), "Did not expect to receive datetime element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		     "rc"), "Expected to receive rc element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "err"), "Did not expect to receive err element");
	zassert_equal(rc, MGMT_ERR_EINVAL, "Expected 'rc' to be invalid value");

	/* Clean up test */
	cleanup_test(NULL);
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;

	/* Query time and ensure it is not set */
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_get_packet(zse, false, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "datetime"), "Did not expect to receive datetime element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		     "rc"), "Did not expect to receive rc element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "err"), "Expected to receive err element");
	zassert_equal(rc, MGMT_ERR_ENOENT, "Expected 'rc' to be no entity");
}

ZTEST(os_mgmt_datetime_not_set, test_datetime_set_invalid_v2_1)
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
	struct group_error group_error;
	int rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("datetime", zcbor_tstr_decode, &output),
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &rc),
		ZCBOR_MAP_DECODE_KEY_DECODER("err", mcumgr_ret_decode, &group_error),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 4, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_set_packet_str(zse, true, invalid_time2_string, buffer,
						   buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "datetime"), "Did not expect to receive datetime element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		     "rc"), "Expected to receive rc element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "err"), "Did not expect to receive err element");
	zassert_equal(rc, MGMT_ERR_EINVAL, "Expected 'rc' to be invalid value");

	/* Clean up test */
	cleanup_test(NULL);
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;

	/* Query time and ensure it is not set */
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_get_packet(zse, true, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "datetime"), "Did not expect to receive datetime element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "rc"), "Did not expect to receive rc element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		     "err"), "Expected to receive err element");
	zassert_equal(group_error.group, MGMT_GROUP_ID_OS, "Expected 'err' -> 'group' to be OS");
	zassert_equal(group_error.rc, OS_MGMT_ERR_RTC_NOT_SET,
		      "Expected 'err' -> 'rc' to be RTC not set");
}

ZTEST(os_mgmt_datetime_not_set, test_datetime_set_invalid_v2_2)
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
	struct group_error group_error;
	int rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("datetime", zcbor_tstr_decode, &output),
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &rc),
		ZCBOR_MAP_DECODE_KEY_DECODER("err", mcumgr_ret_decode, &group_error),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 4, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_set_packet_str(zse, true, invalid_time2_string, buffer,
						   buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "datetime"), "Did not expect to receive datetime element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		     "rc"), "Expected to receive rc element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "err"), "Did not expect to receive err element");
	zassert_equal(rc, MGMT_ERR_EINVAL, "Expected 'rc' to be invalid value");

	/* Clean up test */
	cleanup_test(NULL);
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;

	/* Query time and ensure it is not set */
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_get_packet(zse, true, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "datetime"), "Did not expect to receive datetime element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "rc"), "Did not expect to receive rc element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		     "err"), "Expected to receive err element");
	zassert_equal(group_error.group, MGMT_GROUP_ID_OS, "Expected 'err' -> 'group' to be OS");
	zassert_equal(group_error.rc, OS_MGMT_ERR_RTC_NOT_SET,
		      "Expected 'err' -> 'rc' to be RTC not set");
}

ZTEST(os_mgmt_datetime_not_set, test_datetime_set_invalid_v2_3)
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
	struct group_error group_error;
	int rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("datetime", zcbor_tstr_decode, &output),
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &rc),
		ZCBOR_MAP_DECODE_KEY_DECODER("err", mcumgr_ret_decode, &group_error),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 4, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_set_packet_str(zse, true, invalid_time3_string, buffer,
						   buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "datetime"), "Did not expect to receive datetime element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		     "rc"), "Expected to receive rc element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "err"), "Did not expect to receive err element");
	zassert_equal(rc, MGMT_ERR_EINVAL, "Expected 'rc' to be invalid value");

	/* Clean up test */
	cleanup_test(NULL);
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;

	/* Query time and ensure it is not set */
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_get_packet(zse, true, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "datetime"), "Did not expect to receive datetime element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "rc"), "Did not expect to receive rc element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		     "err"), "Expected to receive err element");
	zassert_equal(group_error.group, MGMT_GROUP_ID_OS, "Expected 'err' -> 'group' to be OS");
	zassert_equal(group_error.rc, OS_MGMT_ERR_RTC_NOT_SET,
		      "Expected 'err' -> 'rc' to be RTC not set");
}

ZTEST(os_mgmt_datetime_set, test_datetime_set_v1)
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
	struct group_error group_error;
	int rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("datetime", zcbor_tstr_decode, &output),
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &rc),
		ZCBOR_MAP_DECODE_KEY_DECODER("err", mcumgr_ret_decode, &group_error),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 4, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_set_packet(zse, false, &valid_time, buffer, buffer_out,
					       &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 0, "Did not expect to receive any decoded zcbor element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "datetime"), "Did not expect to receive datetime element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "rc"), "Did not expect to receive rc element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "err"), "Did not expect to receive err element");

	/* Clean up test */
	cleanup_test(NULL);
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;

	/* Query time and ensure it is set */
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_get_packet(zse, false, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		     "datetime"), "Expected to receive datetime element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "rc"), "Did not expect to receive rc element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "err"), "Did not expected to receive err element");

	/* Check that the date/time is as expected */
	zassert_equal(output.len, strlen(valid_time_string),
		      "Expected received datetime length mismatch");
	zassert_mem_equal(output.value, valid_time_string, strlen(valid_time_string),
			  "Expected received datetime value mismatch");
}

ZTEST(os_mgmt_datetime_set, test_datetime_set_v2)
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
	struct group_error group_error;
	int rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("datetime", zcbor_tstr_decode, &output),
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &rc),
		ZCBOR_MAP_DECODE_KEY_DECODER("err", mcumgr_ret_decode, &group_error),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 4, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_set_packet(zse, true, &valid_time2, buffer, buffer_out,
					       &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 0, "Did not expect to receive any decoded zcbor element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "datetime"), "Did not expect to receive datetime element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "rc"), "Did not expect to receive rc element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "err"), "Did not expect to receive err element");

	/* Clean up test */
	cleanup_test(NULL);
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;

	/* Query time and ensure it is set */
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_get_packet(zse, false, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		     "datetime"), "Expected to receive datetime element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "rc"), "Did not expect to receive rc element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "err"), "Did not expected to receive err element");

	/* Check that the date/time is as expected */
	zassert_equal(output.len, strlen(valid_time2_string),
		      "Expected received datetime length mismatch");
	zassert_mem_equal(output.value, valid_time2_string, strlen(valid_time2_string),
			  "Expected received datetime value mismatch");
}

#ifdef CONFIG_MCUMGR_GRP_OS_DATETIME_HOOK
static void *setup_os_datetime_callbacks(void)
{
	mgmt_callback_register(&os_datetime_callbacks);
	return NULL;
}

static void destroy_os_datetime_callbacks(void *p)
{
	mgmt_callback_unregister(&os_datetime_callbacks);
}

ZTEST(os_mgmt_datetime_hook, test_datetime_set_valid_hook_v1)
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
	struct group_error group_error;
	int rc;
	struct rtc_time *hook_data;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("datetime", zcbor_tstr_decode, &output),
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &rc),
		ZCBOR_MAP_DECODE_KEY_DECODER("err", mcumgr_ret_decode, &group_error),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 4, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_set_packet(zse, false, &valid_time2, buffer, buffer_out,
					       &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "datetime"), "Did not expect to receive datetime element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		     "rc"), "Expected to receive rc element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "err"), "Did not expect to receive err element");
	zassert_equal(rc, MGMT_ERR_EACCESSDENIED, "Expected 'rc' to be access denied");

	/* Check hook actions are as expected */
	hook_data = (struct rtc_time *)hook_set_data;
	zassert_false(hook_get_ran, "Did not expect get hook to run");
	zassert_true(hook_set_ran, "Expected set hook to run");
	zassert_false(hook_other_ran, "Did not expect other hooks to run");
	zassert_equal(hook_set_data_size, sizeof(valid_time2),
		      "Expected data size to match time struct size");
	zassert_equal(valid_time2.tm_sec, hook_data->tm_sec, "Expected value mismatch");
	zassert_equal(valid_time2.tm_min, hook_data->tm_min, "Expected value mismatch");
	zassert_equal(valid_time2.tm_hour, hook_data->tm_hour, "Expected value mismatch");
	zassert_equal(valid_time2.tm_mday, hook_data->tm_mday, "Expected value mismatch");
	zassert_equal(valid_time2.tm_mon, (hook_data->tm_mon + 1), "Expected value mismatch");
	zassert_equal(valid_time2.tm_year, (hook_data->tm_year + 1900),
		      "Expected value mismatch");

	/* Clean up test */
	cleanup_test(NULL);
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;

	/* Query time and ensure it is not set */
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_get_packet(zse, false, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "datetime"), "Did not expect to receive datetime element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		     "rc"), "Expected to receive rc element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "err"), "Did not expect to receive err element");
	zassert_equal(rc, MGMT_ERR_EBUSY, "Expected 'rc' to be busy");

	/* Check hook actions are as expected */
	zassert_true(hook_get_ran, "Expected get hook to run");
	zassert_false(hook_set_ran, "Did not expect set hook to run");
	zassert_false(hook_other_ran, "Did not expect other hooks to run");
	zassert_equal(hook_set_data_size, 0, "Expected data size to be 0");
}

ZTEST(os_mgmt_datetime_hook, test_datetime_set_valid_hook_v2)
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
	struct group_error group_error;
	int rc;
	struct rtc_time *hook_data;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("datetime", zcbor_tstr_decode, &output),
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &rc),
		ZCBOR_MAP_DECODE_KEY_DECODER("err", mcumgr_ret_decode, &group_error),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 4, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_set_packet(zse, true, &valid_time2, buffer, buffer_out,
					       &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "datetime"), "Did not expect to receive datetime element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		     "rc"), "Expected to receive rc element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "err"), "Did not expect to receive err element");
	zassert_equal(rc, MGMT_ERR_EACCESSDENIED, "Expected 'rc' to be access denied");

	/* Check hook actions are as expected */
	hook_data = (struct rtc_time *)hook_set_data;
	zassert_false(hook_get_ran, "Did not expect get hook to run");
	zassert_true(hook_set_ran, "Expected set hook to run");
	zassert_false(hook_other_ran, "Did not expect other hooks to run");
	zassert_equal(hook_set_data_size, sizeof(valid_time2),
		      "Expected data size to match time struct size");
	zassert_equal(valid_time2.tm_sec, hook_data->tm_sec, "Expected value mismatch");
	zassert_equal(valid_time2.tm_min, hook_data->tm_min, "Expected value mismatch");
	zassert_equal(valid_time2.tm_hour, hook_data->tm_hour, "Expected value mismatch");
	zassert_equal(valid_time2.tm_mday, hook_data->tm_mday, "Expected value mismatch");
	zassert_equal(valid_time2.tm_mon, (hook_data->tm_mon + 1), "Expected value mismatch");
	zassert_equal(valid_time2.tm_year, (hook_data->tm_year + 1900), "Expected value mismatch");

	/* Clean up test */
	cleanup_test(NULL);
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;

	/* Query time and ensure it is not set */
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_mcumgr_datetime_get_packet(zse, true, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "datetime"), "Did not expect to receive datetime element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		     "rc"), "Expected to receive rc element");
	zassert_false(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
		      "err"), "Did not expect to receive err element");
	zassert_equal(rc, MGMT_ERR_EBUSY, "Expected 'rc' to be busy");

	/* Check hook actions are as expected */
	zassert_true(hook_get_ran, "Expected get hook to run");
	zassert_false(hook_set_ran, "Did not expect set hook to run");
	zassert_false(hook_other_ran, "Did not expect other hooks to run");
	zassert_equal(hook_set_data_size, 0, "Expected data size to be 0");
}
#endif

static void cleanup_test(void *p)
{
	if (nb != NULL) {
		net_buf_unref(nb);
		nb = NULL;
	}

#ifdef CONFIG_MCUMGR_GRP_OS_DATETIME_HOOK
	hook_get_ran = false;
	hook_set_ran = false;
	hook_other_ran = false;
	hook_set_data_size = 0;
	hook_set_data[0] = 0;
#endif
}

void test_main(void)
{
	while (test_state.test_set < OS_MGMT_DATETIME_TEST_SET_COUNT) {
		ztest_run_all(&test_state, false, 1, 1);
		++test_state.test_set;
	}

	ztest_verify_all_test_suites_ran();
}

static bool time_not_set_predicate(const void *state)
{
	return ((struct state *)state)->test_set == OS_MGMT_DATETIME_TEST_SET_TIME_NOT_SET;
}

static bool time_set_predicate(const void *state)
{
	return ((struct state *)state)->test_set == OS_MGMT_DATETIME_TEST_SET_TIME_SET;
}

#ifdef CONFIG_MCUMGR_GRP_OS_DATETIME_HOOK
static bool hooks_predicate(const void *state)
{
	return ((struct state *)state)->test_set == OS_MGMT_DATETIME_TEST_SET_HOOKS;
}
#endif

/* Time not set test set */
ZTEST_SUITE(os_mgmt_datetime_not_set, time_not_set_predicate, NULL, NULL, cleanup_test, NULL);

#ifdef CONFIG_MCUMGR_GRP_OS_DATETIME_HOOK
/* Hook test set */
ZTEST_SUITE(os_mgmt_datetime_hook, hooks_predicate, setup_os_datetime_callbacks, NULL,
	    cleanup_test, destroy_os_datetime_callbacks);
#endif

/* Time set test set */
ZTEST_SUITE(os_mgmt_datetime_set, time_set_predicate, NULL, NULL, cleanup_test, NULL);
