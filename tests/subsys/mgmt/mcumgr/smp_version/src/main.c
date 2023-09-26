/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net/buf.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/transport/smp_dummy.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <string.h>
#include <smp_internal.h>
#include "smp_test_util.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mcumgr_fs_grp, 4);

enum {
	LEGACY_VERSION = 0,
	CURRENT_VERSION,
	FUTURE_VERSION,
};

#define SMP_RESPONSE_WAIT_TIME 3
#define QUERY_BUFFER_SIZE 16
#define ZCBOR_BUFFER_SIZE 256
#define OUTPUT_BUFFER_SIZE 256
#define ZCBOR_HISTORY_ARRAY_SIZE 4

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
/* Response to legacy packet */
static const uint8_t response_old[] = {
	0x01, 0x00, 0x00, 0x06, 0x00, 0x00, 0x01, 0x07,
	0xbf, 0x62, 0x72, 0x63, 0x03, 0xff
};
#else
/* Response if a legacy packet is sent and server does not support it */
static const uint8_t response_old[] = {
	0x01, 0x00, 0x00, 0x06, 0x00, 0x00, 0x01, 0x07,
	0xbf, 0x62, 0x72, 0x63, 0x0c, 0xff
};
#endif

/* Response to current packet */
static const uint8_t response_current[] = {
	0x09, 0x00, 0x00, 0x13, 0x00, 0x00, 0x01, 0x07,
	0xbf, 0x63, 0x65, 0x72, 0x72, 0xbf, 0x65, 0x67,
	0x72, 0x6f, 0x75, 0x70, 0x00, 0x62, 0x72, 0x63,
	0x02, 0xff, 0xff
};

/* Response if an invalid (too high) version packet is sent and server does not support it */
static const uint8_t response_new[] = {
	0x09, 0x00, 0x00, 0x06, 0x00, 0x00, 0x01, 0x07,
	0xbf, 0x62, 0x72, 0x63, 0x0d, 0xff
};

static struct net_buf *nb;

const uint8_t query_fake[] = "8";

struct group_error {
	uint16_t group;
	uint16_t rc;
	bool found;
};

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

ZTEST(smp_version, test_legacy_command)
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
	struct smp_hdr *smp_header;
	int32_t rc;
	struct group_error group_error;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &rc),
		ZCBOR_MAP_DECODE_KEY_DECODER("err", mcumgr_ret_decode, &group_error),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, query_fake, buffer, buffer_out, &buffer_size,
					 LEGACY_VERSION);
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

	/* Check that the received response matches the expected response */
	zassert_equal(sizeof(response_old), nb->len, "Expected received data length mismatch");
	zassert_mem_equal(response_old, nb->data, nb->len, "Expected received data mismatch");

	/* Process received data by removing header */
	smp_header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));
	zassert_equal(smp_header->nh_version, LEGACY_VERSION,
		      "Expected response header version mismatch");

	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1);
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	zassert_equal(
		zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"output"), 0,
		"Did not expect to get output in response");

	zassert_equal(
		zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"rc"), 1,
		"Expected to get rc in response");

	zassert_equal(
		zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"err"), 0,
		"Did not expect to get ret in response");

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
	zassert_equal(rc, MGMT_ERR_EINVAL, "Expected to get MGMT_ERR_EINVAL error");
#else
	zassert_equal(rc, MGMT_ERR_UNSUPPORTED_TOO_OLD,
		      "Expected to get MGMT_ERR_UNSUPPORTED_TOO_OLD error");
#endif
}

ZTEST(smp_version, test_current_command)
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
	struct smp_hdr *smp_header;
	int32_t rc;
	struct group_error group_error;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &rc),
		ZCBOR_MAP_DECODE_KEY_DECODER("err", mcumgr_ret_decode, &group_error),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, query_fake, buffer, buffer_out, &buffer_size,
					 CURRENT_VERSION);
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

	/* Check that the received response matches the expected response */
	zassert_equal(sizeof(response_current), nb->len, "Expected received data length mismatch");
	zassert_mem_equal(response_current, nb->data, nb->len, "Expected received data mismatch");

	/* Process received data by removing header */
	smp_header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));
	zassert_equal(smp_header->nh_version, CURRENT_VERSION,
		      "Expected response header version mismatch");

	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1);
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	zassert_equal(
		zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"output"), 0,
		"Did not expect to get output in response");

	zassert_equal(
		zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"rc"), 0,
		"Did not expect to get rc in response");

	zassert_equal(
		zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"err"), 1,
		"Expected to get ret in response");

	zassert_true(group_error.found, "Expected both group and rc in ret to be found");
	zassert_equal(group_error.group, MGMT_GROUP_ID_OS,
		      "Expected to get MGMT_GROUP_ID_OS for ret group");
	zassert_equal(group_error.rc, OS_MGMT_ERR_INVALID_FORMAT,
		      "Expected to get OS_MGMT_ERR_INVALID_FORMAT for ret rc");
}

ZTEST(smp_version, test_new_command)
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
	struct smp_hdr *smp_header;
	int32_t rc;
	struct group_error group_error;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &rc),
		ZCBOR_MAP_DECODE_KEY_DECODER("err", mcumgr_ret_decode, &group_error),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, query_fake, buffer, buffer_out, &buffer_size,
					 FUTURE_VERSION);
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

	/* Check that the received response matches the expected response */
	zassert_equal(sizeof(response_new), nb->len, "Expected received data length mismatch");
	zassert_mem_equal(response_new, nb->data, nb->len, "Expected received data mismatch");

	/* Process received data by removing header */
	smp_header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));
	zassert_equal(smp_header->nh_version, CURRENT_VERSION,
		      "Expected response header version mismatch");

	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1);
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	zassert_equal(
		zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"output"), 0,
		"Did not expect to get output in response");

	zassert_equal(
		zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"rc"), 1,
		"Expected to get rc in response");

	zassert_equal(
		zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"err"), 0,
		"Did not expect to get ret in response");

	zassert_equal(rc, MGMT_ERR_UNSUPPORTED_TOO_NEW,
		      "Expected to get MGMT_ERR_UNSUPPORTED_TOO_NEW error");
}

static void cleanup_test(void *p)
{
	if (nb != NULL) {
		net_buf_unref(nb);
		nb = NULL;
	}
}

/* Main test set */
ZTEST_SUITE(smp_version, NULL, NULL, NULL, cleanup_test, NULL);
