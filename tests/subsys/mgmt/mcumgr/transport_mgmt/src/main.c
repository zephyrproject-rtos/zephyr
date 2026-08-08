/*
 * Copyright (c) 2023-2026 Nordic Semiconductor ASA
 * Copyright (c) 2026, Jamie McCrae
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net_buf.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/transport/smp_dummy.h>
#include <zephyr/mgmt/mcumgr/transport/smp_raw_dummy.h>
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/transport_mgmt/transport_mgmt.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <limits.h>
#include "smp_test_util.h"

#define SMP_RESPONSE_WAIT_TIME 3
#define ZCBOR_BUFFER_SIZE 128
#define OUTPUT_BUFFER_SIZE 384
#define ZCBOR_HISTORY_ARRAY_SIZE 8
#define TEST_STRING_RAW "rAW datA TEsT"
#define TEST_STRING "noRMAL d4t4 7est"

struct group_error_t {
	uint16_t group;
	uint16_t rc;
	bool found;
};

struct transport_mgmt_modes {
	uint32_t id;
	struct zcbor_string description;
	bool incoming;
	bool incoming_found;
	bool outgoing;
	bool outgoing_found;
	bool found;
};

struct transport_mgmt_lists {
	uint32_t transport;
	struct zcbor_string name;
	bool found;
};

struct transport_mgmt_configs {
	struct zcbor_string name;
	uint32_t type;
	bool required;
	bool found;
};

static struct net_buf *nb;

static void cleanup_test(void *p)
{
	if (nb != NULL) {
		net_buf_reset(nb);
		net_buf_unref(nb);
		nb = NULL;
	}
}

static bool mcumgr_ret_decode(zcbor_state_t *state, struct group_error_t *result)
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

	ok = zcbor_map_decode_bulk(state, output_decode, ARRAY_SIZE(output_decode),
				   &decoded) == 0;

	if (ok &&
	    zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode), "group") &&
	    zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode), "rc")) {
		result->group = (uint16_t)tmp_group;
		result->rc = (uint16_t)tmp_rc;
		result->found = true;
	}

	return ok;
}

static bool transport_mgmt_modes_decode(zcbor_state_t *state, struct transport_mgmt_modes *result)
{
	bool ok;
	size_t decoded;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("id", zcbor_uint32_decode, &result->id),
		ZCBOR_MAP_DECODE_KEY_DECODER("description", zcbor_tstr_decode,
					     &result->description),
		ZCBOR_MAP_DECODE_KEY_DECODER("incoming", zcbor_bool_decode, &result->incoming),
		ZCBOR_MAP_DECODE_KEY_DECODER("outgoing", zcbor_bool_decode, &result->outgoing),
	};

	result->found = false;

	ok = zcbor_list_start_decode(state) &&
	     zcbor_map_decode_bulk(state, output_decode, ARRAY_SIZE(output_decode),
				   &decoded) == 0;

	if (ok &&
	    zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode), "id") &&
	    zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
					    "description")) {
		result->found = true;

		if (zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						    "incoming")) {
			result->incoming_found = true;
		}

		if (zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						    "outgoing")) {
			result->outgoing_found = true;
		}

		ok = zcbor_list_end_decode(state);
	}

	return ok;
}

static bool transport_mgmt_list_decode(zcbor_state_t *state, struct transport_mgmt_lists *result)
{
	bool ok = true;
	size_t decoded;
	uint8_t entries = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("id", zcbor_uint32_decode, &result[0].transport),
		ZCBOR_MAP_DECODE_KEY_DECODER("name", zcbor_tstr_decode, &result[0].name),
	};

	if (!zcbor_list_start_decode(state)) {
		return false;
	}

	while (1) {
		ok = zcbor_map_decode_bulk(state, output_decode, ARRAY_SIZE(output_decode),
					   &decoded) == 0;

		if (ok &&
		    zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						    "id") &&
		    zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						    "name")) {
			result[entries].found = true;
			++entries;
			output_decode[0].value_ptr = &result[entries].transport;
			output_decode[1].value_ptr = &result[entries].name;
			output_decode[0].found = false;
			output_decode[1].found = false;
		} else {
			if (entries > 0) {
				return zcbor_list_end_decode(state);
			}

			return false;
		}
	}
}

static bool transport_mgmt_config_decode(zcbor_state_t *state,
					 struct transport_mgmt_configs *result)
{
	bool ok = true;
	size_t decoded;
	uint8_t entries = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("name", zcbor_tstr_decode, &result[0].name),
		ZCBOR_MAP_DECODE_KEY_DECODER("type", zcbor_uint32_decode, &result[0].type),
		ZCBOR_MAP_DECODE_KEY_DECODER("required", zcbor_bool_decode, &result[0].required),
	};

	if (!zcbor_list_start_decode(state)) {
		return false;
	}

	while (1) {
		ok = zcbor_map_decode_bulk(state, output_decode, ARRAY_SIZE(output_decode),
					   &decoded) == 0;

		if (ok &&
		    zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						    "name") &&
		    zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						    "type")) {
			result[entries].found = true;
			++entries;
			output_decode[0].value_ptr = &result[entries].name;
			output_decode[1].value_ptr = &result[entries].type;
			output_decode[2].value_ptr = &result[entries].required;
			output_decode[0].found = false;
			output_decode[1].found = false;
			output_decode[2].found = false;
		} else {
			if (entries > 0) {
				return zcbor_list_end_decode(state);
			}

			return false;
		}
	}
}

ZTEST(transport_mgmt, test_connect_invalid)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct smp_hdr *header;
	size_t decoded = 0;
	struct group_error_t group_error;
	int rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("err", mcumgr_ret_decode, &group_error),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	/* Test 1: Send overly high transport to bridge to (47) */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_connect_packet(zse, buffer, buffer_out, &buffer_size, 47);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_CONNECT, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(group_error.group, MGMT_GROUP_ID_TRANSPORT,
		      "Expected 'err' -> 'group' to be transport");
	zassert_equal(group_error.rc, TRANSPORT_MGMT_ERR_INVALID_TRANSPORT,
		      "Expected 'err' -> 'rc' to be invalid transport");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	cleanup_test(NULL);

	/* Test 2: Send low transport to bridge to (2) */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_connect_packet(zse, buffer, buffer_out, &buffer_size, 2);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_CONNECT, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(group_error.group, MGMT_GROUP_ID_TRANSPORT,
		      "Expected 'err' -> 'group' to be transport");
	zassert_equal(group_error.rc, TRANSPORT_MGMT_ERR_INVALID_TRANSPORT,
		      "Expected 'err' -> 'rc' to be invalid transport");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	cleanup_test(NULL);

	/* Test 3: Send same transport ID as is being used (0) */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_connect_packet(zse, buffer, buffer_out, &buffer_size, 0);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_CONNECT, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(group_error.group, MGMT_GROUP_ID_TRANSPORT,
		      "Expected 'err' -> 'group' to be transport");
	zassert_equal(group_error.rc, TRANSPORT_MGMT_ERR_SAME_BRIDGE_DEVICE_DISALLOWED,
		      "Expected 'err' -> 'rc' to be same bridge device disallowed");
}

ZTEST(transport_mgmt, test_disconnect_invalid)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct smp_hdr *header;
	size_t decoded = 0;
	struct group_error_t group_error;
	int rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("err", mcumgr_ret_decode, &group_error),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	/* Test 1: Disconnect all bridges with no bridge active */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_disconnect_packet(zse, buffer, buffer_out, &buffer_size, 0,
						     true);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_DISCONNECT,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(group_error.group, MGMT_GROUP_ID_TRANSPORT,
		      "Expected 'err' -> 'group' to be transport");
	zassert_equal(group_error.rc, TRANSPORT_MGMT_ERR_NOT_BRIDGED,
		      "Expected 'err' -> 'rc' to be not bridged");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	cleanup_test(NULL);

	/* Test 2: Disconnect one bridge with no bridge active */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_disconnect_packet(zse, buffer, buffer_out, &buffer_size, 1,
						     false);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_DISCONNECT,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(group_error.group, MGMT_GROUP_ID_TRANSPORT,
		      "Expected 'err' -> 'group' to be transport");
	zassert_equal(group_error.rc, TRANSPORT_MGMT_ERR_NOT_BRIDGED,
		      "Expected 'err' -> 'rc' to be not bridged");
}

/*
 * Tests commands work and get responses without bridge, then tests bridging and un-bridging works
 * along with data transfer between the transport
 */
ZTEST(transport_mgmt, test_connection)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct smp_hdr *header;
	size_t decoded = 0;
	struct zcbor_string echo_send_data = { 0 };
	struct zcbor_string echo_receive_data = { 0 };
	struct group_error_t group_error;
	uint32_t rc = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("d", zcbor_tstr_decode, &echo_send_data),
		ZCBOR_MAP_DECODE_KEY_DECODER("r", zcbor_tstr_decode, &echo_receive_data),
		ZCBOR_MAP_DECODE_KEY_DECODER("err", mcumgr_ret_decode, &group_error),
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_uint32_decode, &rc),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	/* Test 1: Check dummy transport does echo and gets response */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_echo_packet(zse, buffer, buffer_out, &buffer_size, TEST_STRING);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, OS_MGMT_ID_ECHO, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(echo_receive_data.len, strlen(TEST_STRING),
		      "os mgmt echo response length mismatch");
	zassert_mem_equal(echo_receive_data.value, TEST_STRING, echo_receive_data.len,
			  "os mgmt echo response mismatch");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;
	output_decode[3].found = false;
	echo_send_data.len = 0;
	echo_receive_data.len = 0;
	cleanup_test(NULL);

	/* Test 2: Check raw dummy transport does echo and gets response */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_echo_packet(zse, buffer, buffer_out, &buffer_size, TEST_STRING_RAW);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_raw_dummy_enable();
	smp_raw_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_raw_dummy_tx_pkt(buffer_out, buffer_size);
	smp_raw_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_raw_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_raw_dummy_get_outgoing();
	smp_raw_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, OS_MGMT_ID_ECHO, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful %d", rc);
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(echo_receive_data.len, strlen(TEST_STRING_RAW),
		      "os mgmt echo response length mismatch");
	zassert_mem_equal(echo_receive_data.value, TEST_STRING_RAW, echo_receive_data.len,
			  "os mgmt echo response mismatch");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;
	output_decode[3].found = false;
	echo_send_data.len = 0;
	echo_receive_data.len = 0;
	cleanup_test(NULL);

	/* Test 3: Bridge to other raw dummy transport */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_connect_packet(zse, buffer, buffer_out, &buffer_size, 1);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_CONNECT, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 0, "Expected to receive 0 decoded zcbor elements");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;
	output_decode[3].found = false;
	echo_send_data.len = 0;
	echo_receive_data.len = 0;
	cleanup_test(NULL);

	/* Test 4: Check dummy transport does echo and raw dummy gets response */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_echo_packet(zse, buffer, buffer_out, &buffer_size, TEST_STRING);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();
	smp_raw_dummy_enable();
	smp_raw_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* Ensure primary transport gets no response */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_false(received, "Expected to not receive data but received it");

	/* For a short duration to see if response has been received */
	received = smp_raw_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_raw_dummy_get_outgoing();
	smp_dummy_disable();
	smp_raw_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, OS_MGMT_ID_ECHO, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(echo_send_data.len, strlen(TEST_STRING),
		      "os mgmt echo response length mismatch");
	zassert_mem_equal(echo_send_data.value, TEST_STRING, echo_send_data.len,
			  "os mgmt echo response mismatch");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;
	output_decode[3].found = false;
	echo_send_data.len = 0;
	echo_receive_data.len = 0;
	cleanup_test(NULL);

	/* Test 5: Check raw dummy transport does echo and dummy gets response */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_echo_response_packet(zse, buffer, buffer_out, &buffer_size,
						 TEST_STRING_RAW);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_raw_dummy_enable();
	smp_raw_dummy_clear_state();
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_raw_dummy_tx_pkt(buffer_out, buffer_size);
	smp_raw_dummy_add_data();

	/* Ensure primary transport gets no response */
	received = smp_raw_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_false(received, "Expected to not receive data but received it");

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_raw_dummy_disable();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, OS_MGMT_ID_ECHO, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(echo_send_data.len, strlen(TEST_STRING_RAW),
		      "os mgmt echo response length mismatch");
	zassert_mem_equal(echo_send_data.value, TEST_STRING_RAW, echo_send_data.len,
			  "os mgmt echo response mismatch");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;
	output_decode[3].found = false;
	echo_send_data.len = 0;
	echo_receive_data.len = 0;
	cleanup_test(NULL);

	/* Test 6: Disconnect one bridge with bridge active */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_disconnect_packet(zse, buffer, buffer_out, &buffer_size, 1,
						     false);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_DISCONNECT,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 0, "Expected to receive 0 decoded zcbor elements");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;
	output_decode[3].found = false;
	echo_send_data.len = 0;
	echo_receive_data.len = 0;
	cleanup_test(NULL);

	/* Test 7: Check dummy transport does echo and gets response */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_echo_packet(zse, buffer, buffer_out, &buffer_size, TEST_STRING);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, OS_MGMT_ID_ECHO, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(echo_receive_data.len, strlen(TEST_STRING),
		      "os mgmt echo response length mismatch");
	zassert_mem_equal(echo_receive_data.value, TEST_STRING, echo_receive_data.len,
			  "os mgmt echo response mismatch");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;
	output_decode[3].found = false;
	echo_send_data.len = 0;
	echo_receive_data.len = 0;
	cleanup_test(NULL);

	/* Test 8: Check raw dummy transport does echo and gets response */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_echo_packet(zse, buffer, buffer_out, &buffer_size, TEST_STRING_RAW);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_raw_dummy_enable();
	smp_raw_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_raw_dummy_tx_pkt(buffer_out, buffer_size);
	smp_raw_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_raw_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_raw_dummy_get_outgoing();
	smp_raw_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, OS_MGMT_ID_ECHO, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful %d", rc);
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(echo_receive_data.len, strlen(TEST_STRING_RAW),
		      "os mgmt echo response length mismatch");
	zassert_mem_equal(echo_receive_data.value, TEST_STRING_RAW, echo_receive_data.len,
			  "os mgmt echo response mismatch");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;
	output_decode[3].found = false;
	echo_send_data.len = 0;
	echo_receive_data.len = 0;
	cleanup_test(NULL);

	/* Test 9: Bridge to other dummy transport */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_connect_packet(zse, buffer, buffer_out, &buffer_size, 0);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_raw_dummy_enable();
	smp_raw_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_raw_dummy_tx_pkt(buffer_out, buffer_size);
	smp_raw_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_raw_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_raw_dummy_get_outgoing();
	smp_raw_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_CONNECT, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 0, "Expected to receive 0 decoded zcbor elements");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;
	output_decode[3].found = false;
	echo_send_data.len = 0;
	echo_receive_data.len = 0;
	cleanup_test(NULL);

	/* Test 10: Check dummy transport does echo and raw dummy gets response */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_echo_response_packet(zse, buffer, buffer_out, &buffer_size,
						 TEST_STRING);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();
	smp_raw_dummy_enable();
	smp_raw_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* Ensure primary transport gets no response */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_false(received, "Expected to not receive data but received it");

	/* For a short duration to see if response has been received */
	received = smp_raw_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_raw_dummy_get_outgoing();
	smp_dummy_disable();
	smp_raw_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, OS_MGMT_ID_ECHO, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(echo_send_data.len, strlen(TEST_STRING),
		      "os mgmt echo response length mismatch");
	zassert_mem_equal(echo_send_data.value, TEST_STRING, echo_send_data.len,
			  "os mgmt echo response mismatch");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;
	output_decode[3].found = false;
	echo_send_data.len = 0;
	echo_receive_data.len = 0;
	cleanup_test(NULL);

	/* Test 11: Check raw dummy transport does echo and dummy gets response */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_echo_packet(zse, buffer, buffer_out, &buffer_size, TEST_STRING_RAW);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_raw_dummy_enable();
	smp_raw_dummy_clear_state();
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_raw_dummy_tx_pkt(buffer_out, buffer_size);
	smp_raw_dummy_add_data();

	/* Ensure primary transport gets no response */
	received = smp_raw_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_false(received, "Expected to not receive data but received it");

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_raw_dummy_disable();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, OS_MGMT_ID_ECHO, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(echo_send_data.len, strlen(TEST_STRING_RAW),
		      "os mgmt echo response length mismatch");
	zassert_mem_equal(echo_send_data.value, TEST_STRING_RAW, echo_send_data.len,
			  "os mgmt echo response mismatch");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;
	output_decode[3].found = false;
	echo_send_data.len = 0;
	echo_receive_data.len = 0;
	cleanup_test(NULL);

	/* Test 12: Disconnect all bridges with one active bridge */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_disconnect_packet(zse, buffer, buffer_out, &buffer_size, 0,
						     true);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_raw_dummy_enable();
	smp_raw_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_raw_dummy_tx_pkt(buffer_out, buffer_size);
	smp_raw_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_raw_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_raw_dummy_get_outgoing();
	smp_raw_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_DISCONNECT,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 0, "Expected to receive 0 decoded zcbor elements");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;
	output_decode[3].found = false;
	echo_send_data.len = 0;
	echo_receive_data.len = 0;
	cleanup_test(NULL);

	/* Test 13: Check dummy transport does echo and gets response */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_echo_packet(zse, buffer, buffer_out, &buffer_size, TEST_STRING);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, OS_MGMT_ID_ECHO, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(echo_receive_data.len, strlen(TEST_STRING),
		      "os mgmt echo response length mismatch");
	zassert_mem_equal(echo_receive_data.value, TEST_STRING, echo_receive_data.len,
			  "os mgmt echo response mismatch");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;
	output_decode[3].found = false;
	echo_send_data.len = 0;
	echo_receive_data.len = 0;
	cleanup_test(NULL);

	/* Test 14: Check raw dummy transport does echo and gets response */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_echo_packet(zse, buffer, buffer_out, &buffer_size, TEST_STRING_RAW);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_raw_dummy_enable();
	smp_raw_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_raw_dummy_tx_pkt(buffer_out, buffer_size);
	smp_raw_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_raw_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_raw_dummy_get_outgoing();
	smp_raw_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, OS_MGMT_ID_ECHO, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful %d", rc);
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(echo_receive_data.len, strlen(TEST_STRING_RAW),
		      "os mgmt echo response length mismatch");
	zassert_mem_equal(echo_receive_data.value, TEST_STRING_RAW, echo_receive_data.len,
			  "os mgmt echo response mismatch");
}

/*
 * Bridge transports, test wrong type and ensure response is received on _same_ port not on
 * bridged one
 */
ZTEST(transport_mgmt, test_connection_reverse_incoming)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct smp_hdr *header;
	size_t decoded = 0;
	struct zcbor_string echo_receive_data = { 0 };
	uint32_t rc = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("r", zcbor_tstr_decode, &echo_receive_data),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	/* Test 1: Bridge to other raw dummy transport */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_connect_packet(zse, buffer, buffer_out, &buffer_size, 1);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_CONNECT, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 0, "Expected to receive 0 decoded zcbor elements");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	echo_receive_data.len = 0;
	cleanup_test(NULL);

	/* Test 2: Check dummy transport gets no echo response response */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_echo_response_packet(zse, buffer, buffer_out, &buffer_size,
						 TEST_STRING);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();
	smp_raw_dummy_enable();
	smp_raw_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* Ensure neither transport gets a response */
	received = smp_raw_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_false(received, "Expected to not receive data but received it");
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_false(received, "Expected to not receive data but received it");
	smp_dummy_disable();
	smp_raw_dummy_disable();

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	echo_receive_data.len = 0;
	cleanup_test(NULL);

	/* Test 3: Check raw dummy transport does echo gets response */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_echo_packet(zse, buffer, buffer_out, &buffer_size, TEST_STRING_RAW);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_raw_dummy_enable();
	smp_raw_dummy_clear_state();
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_raw_dummy_tx_pkt(buffer_out, buffer_size);
	smp_raw_dummy_add_data();

	/* Ensure secondary transport gets no response */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_false(received, "Expected to not receive data but received it");

	/* For a short duration to see if response has been received */
	received = smp_raw_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_raw_dummy_get_outgoing();
	smp_raw_dummy_disable();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, OS_MGMT_ID_ECHO, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(echo_receive_data.len, strlen(TEST_STRING_RAW),
		      "os mgmt echo response length mismatch");
	zassert_mem_equal(echo_receive_data.value, TEST_STRING_RAW, echo_receive_data.len,
			  "os mgmt echo response mismatch");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	echo_receive_data.len = 0;
	cleanup_test(NULL);

	/* Test 4: Disconnect all bridges */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_disconnect_packet(zse, buffer, buffer_out, &buffer_size, 0,
						     true);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_raw_dummy_enable();
	smp_raw_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_raw_dummy_tx_pkt(buffer_out, buffer_size);
	smp_raw_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_raw_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_raw_dummy_get_outgoing();
	smp_raw_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_DISCONNECT,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 0, "Expected to receive 0 decoded zcbor elements");
}

ZTEST(transport_mgmt, test_status)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct smp_hdr *header;
	size_t decoded = 0;
	int rc;
	uint32_t supported = UINT32_MAX;
	uint32_t active = UINT32_MAX;
	bool bridged = true;
	uint32_t transport = UINT32_MAX;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("supported", zcbor_uint32_decode, &supported),
		ZCBOR_MAP_DECODE_KEY_DECODER("active", zcbor_uint32_decode, &active),
		ZCBOR_MAP_DECODE_KEY_DECODER("bridged", zcbor_bool_decode, &bridged),
		ZCBOR_MAP_DECODE_KEY_DECODER("transport", zcbor_uint32_decode, &transport),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	/* Test 1: Check status with no bridges from primary transport */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_status_packet(zse, buffer, buffer_out, &buffer_size);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_STATUS, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 2, "Expected to receive 1 decoded zcbor element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"supported"), "Expected 'supported' response");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"active"), "Expected 'active' response");
	zassert_equal(supported, CONFIG_MCUMGR_GRP_TRANSPORT_MAX_BRIDGES,
		      "Expected 'supported' value mismatch");
	zassert_equal(active, 0, "Expected 'active' value mismatch");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;
	output_decode[3].found = false;
	supported = UINT32_MAX;
	active = UINT32_MAX;
	bridged = true;
	transport = UINT32_MAX;
	cleanup_test(NULL);

	/* Test 2: Check status with no bridges from secondary transport */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_status_packet(zse, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_raw_dummy_enable();
	smp_raw_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_raw_dummy_tx_pkt(buffer_out, buffer_size);
	smp_raw_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_raw_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_raw_dummy_get_outgoing();
	smp_raw_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_STATUS, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 2, "Expected to receive 1 decoded zcbor element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"supported"), "Expected 'supported' response");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"active"), "Expected 'active' response");
	zassert_equal(supported, CONFIG_MCUMGR_GRP_TRANSPORT_MAX_BRIDGES,
		      "Expected 'supported' value mismatch");
	zassert_equal(active, 0, "Expected 'active' value mismatch");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;
	output_decode[3].found = false;
	supported = UINT32_MAX;
	active = UINT32_MAX;
	bridged = true;
	transport = UINT32_MAX;
	cleanup_test(NULL);

	/* Test 3: Bridge connection */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_connect_packet(zse, buffer, buffer_out, &buffer_size,
						  SMP_SERIAL_TRANSPORT);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_raw_dummy_enable();
	smp_raw_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_raw_dummy_tx_pkt(buffer_out, buffer_size);
	smp_raw_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_raw_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_raw_dummy_get_outgoing();
	smp_raw_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_CONNECT, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 0, "Expected to receive 0 decoded zcbor elements");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;
	output_decode[3].found = false;
	supported = UINT32_MAX;
	active = UINT32_MAX;
	bridged = false;
	transport = UINT32_MAX;
	cleanup_test(NULL);

	/* Test 4: Check status with one bridge from primary transport */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_status_packet(zse, buffer, buffer_out, &buffer_size);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_STATUS, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 2, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 4, "Expected to receive 4 decoded zcbor element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"supported"), "Expected 'supported' response");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"active"), "Expected 'active' response");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"bridged"), "Expected 'bridged' response");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"transport"), "Expected 'transport' response");
	zassert_equal(supported, CONFIG_MCUMGR_GRP_TRANSPORT_MAX_BRIDGES,
		      "Expected 'supported' value mismatch");
	zassert_equal(active, 1, "Expected 'active' value mismatch");
	zassert_true(bridged, "Expected 'bridged' value mismatch");
	zassert_equal(transport, SMP_RAW_SERIAL_TRANSPORT, "Expected 'transport' value mismatch");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;
	output_decode[3].found = false;
	supported = UINT32_MAX;
	active = UINT32_MAX;
	bridged = false;
	transport = UINT32_MAX;
	cleanup_test(NULL);

	/* Test 5: Check status with one bridge from secondary transport */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_status_packet(zse, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_raw_dummy_enable();
	smp_raw_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_raw_dummy_tx_pkt(buffer_out, buffer_size);
	smp_raw_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_raw_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_raw_dummy_get_outgoing();
	smp_raw_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_STATUS, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 2, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 4, "Expected to receive 4 decoded zcbor element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"supported"), "Expected 'supported' response");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"active"), "Expected 'active' response");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"bridged"), "Expected 'bridged' response");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"transport"), "Expected 'transport' response");
	zassert_equal(supported, CONFIG_MCUMGR_GRP_TRANSPORT_MAX_BRIDGES,
		      "Expected 'supported' value mismatch");
	zassert_equal(active, 1, "Expected 'active' value mismatch");
	zassert_true(bridged, "Expected 'bridged' value mismatch");
	zassert_equal(transport, SMP_SERIAL_TRANSPORT, "Expected 'transport' value mismatch");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;
	output_decode[3].found = false;
	supported = UINT32_MAX;
	active = UINT32_MAX;
	bridged = false;
	transport = UINT32_MAX;
	cleanup_test(NULL);

	/* Test 6: Disconnect all bridges with one active bridge */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_disconnect_packet(zse, buffer, buffer_out, &buffer_size, 0,
						     true);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_raw_dummy_enable();
	smp_raw_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_raw_dummy_tx_pkt(buffer_out, buffer_size);
	smp_raw_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_raw_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_raw_dummy_get_outgoing();
	smp_raw_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_DISCONNECT,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 0, "Expected to receive 0 decoded zcbor elements");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;
	output_decode[3].found = false;
	supported = UINT32_MAX;
	active = UINT32_MAX;
	bridged = true;
	transport = UINT32_MAX;
	cleanup_test(NULL);

	/* Test 7: Check status with no bridges from primary transport */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_status_packet(zse, buffer, buffer_out, &buffer_size);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_STATUS, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 2, "Expected to receive 1 decoded zcbor element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"supported"), "Expected 'supported' response");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"active"), "Expected 'active' response");
	zassert_equal(supported, CONFIG_MCUMGR_GRP_TRANSPORT_MAX_BRIDGES,
		      "Expected 'supported' value mismatch");
	zassert_equal(active, 0, "Expected 'active' value mismatch");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	output_decode[1].found = false;
	output_decode[2].found = false;
	output_decode[3].found = false;
	supported = UINT32_MAX;
	active = UINT32_MAX;
	bridged = true;
	transport = UINT32_MAX;
	cleanup_test(NULL);

	/* Test 8: Check status with no bridges from secondary transport */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_status_packet(zse, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_raw_dummy_enable();
	smp_raw_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_raw_dummy_tx_pkt(buffer_out, buffer_size);
	smp_raw_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_raw_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_raw_dummy_get_outgoing();
	smp_raw_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_STATUS, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 2, "Expected to receive 1 decoded zcbor element");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"supported"), "Expected 'supported' response");
	zassert_true(zcbor_map_decode_bulk_key_found(output_decode, ARRAY_SIZE(output_decode),
						"active"), "Expected 'active' response");
	zassert_equal(supported, CONFIG_MCUMGR_GRP_TRANSPORT_MAX_BRIDGES,
		      "Expected 'supported' value mismatch");
	zassert_equal(active, 0, "Expected 'active' value mismatch");
}

ZTEST(transport_mgmt, test_valid_modes)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct smp_hdr *header;
	size_t decoded = 0;
	struct transport_mgmt_modes modes;
	int rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("modes", transport_mgmt_modes_decode, &modes),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	/* Test 1: Query primary transport modes from primary transport */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_modes_packet(zse, buffer, buffer_out, &buffer_size,
						SMP_SERIAL_TRANSPORT);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_GET_MODES,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 6, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(modes.id, 0, "Expected mode 'id' mismatch'");
	zassert_equal(modes.description.len, strlen("Dummy"),
		      "Expected mode 'description' length mismatch");
	zassert_mem_equal(modes.description.value, "Dummy", modes.description.len,
			  "Expected mode 'description' mismatch");
	zassert_true(modes.incoming, "Expected mode 'incoming' mismatch");
	zassert_true(modes.incoming_found, "Expected mode 'incoming' to be found");
	zassert_true(modes.outgoing, "Expected mode 'outgoing' mismatch");
	zassert_true(modes.outgoing_found, "Expected mode 'outgoing' to be found");
	zassert_true(modes.found, "Expected array to be found");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	memset(&modes, 0, sizeof(modes));
	cleanup_test(NULL);

	/* Test 2: Query primary transport modes from secondary transport */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_modes_packet(zse, buffer, buffer_out, &buffer_size,
						SMP_SERIAL_TRANSPORT);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_raw_dummy_enable();
	smp_raw_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_raw_dummy_tx_pkt(buffer_out, buffer_size);
	smp_raw_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_raw_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_raw_dummy_get_outgoing();
	smp_raw_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_GET_MODES,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 6, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(modes.id, 0, "Expected mode 'id' mismatch'");
	zassert_equal(modes.description.len, strlen("Dummy"),
		      "Expected mode 'description' length mismatch");
	zassert_mem_equal(modes.description.value, "Dummy", modes.description.len,
			  "Expected mode 'description' mismatch");
	zassert_true(modes.incoming, "Expected mode 'incoming' mismatch");
	zassert_true(modes.incoming_found, "Expected mode 'incoming' to be found");
	zassert_true(modes.outgoing, "Expected mode 'outgoing' mismatch");
	zassert_true(modes.outgoing_found, "Expected mode 'outgoing' to be found");
	zassert_true(modes.found, "Expected array to be found");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	memset(&modes, 0, sizeof(modes));
	cleanup_test(NULL);

	/* Test 3: Query secondary transport modes from primary transport */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_modes_packet(zse, buffer, buffer_out, &buffer_size,
						SMP_RAW_SERIAL_TRANSPORT);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_GET_MODES,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 6, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(modes.id, 0, "Expected mode 'id' mismatch'");
	zassert_equal(modes.description.len, strlen("Raw dummy"),
		      "Expected mode 'description' length mismatch");
	zassert_mem_equal(modes.description.value, "Raw dummy", modes.description.len,
			  "Expected mode 'description' mismatch");
	zassert_true(modes.incoming, "Expected mode 'incoming' mismatch");
	zassert_true(modes.incoming_found, "Expected mode 'incoming' to be found");
	zassert_true(modes.outgoing, "Expected mode 'outgoing' mismatch");
	zassert_true(modes.outgoing_found, "Expected mode 'outgoing' to be found");
	zassert_true(modes.found, "Expected array to be found");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	memset(&modes, 0, sizeof(modes));
	cleanup_test(NULL);

	/* Test 4: Query secondary transport modes from secondary transport */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_modes_packet(zse, buffer, buffer_out, &buffer_size,
						SMP_RAW_SERIAL_TRANSPORT);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_raw_dummy_enable();
	smp_raw_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_raw_dummy_tx_pkt(buffer_out, buffer_size);
	smp_raw_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_raw_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_raw_dummy_get_outgoing();
	smp_raw_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_GET_MODES,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 6, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(modes.id, 0, "Expected mode 'id' mismatch'");
	zassert_equal(modes.description.len, strlen("Raw dummy"),
		      "Expected mode 'description' length mismatch");
	zassert_mem_equal(modes.description.value, "Raw dummy", modes.description.len,
			  "Expected mode 'description' mismatch");
	zassert_true(modes.incoming, "Expected mode 'incoming' mismatch");
	zassert_true(modes.incoming_found, "Expected mode 'incoming' to be found");
	zassert_true(modes.outgoing, "Expected mode 'outgoing' mismatch");
	zassert_true(modes.outgoing_found, "Expected mode 'outgoing' to be found");
	zassert_true(modes.found, "Expected array to be found");
}

ZTEST(transport_mgmt, test_list)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct smp_hdr *header;
	size_t decoded = 0;
	struct transport_mgmt_lists transports[3] = { 0x0 };
	int rc;
	uint8_t i = 0;
	uint8_t entries = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("transports", transport_mgmt_list_decode, &transports),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	/* Test 1: Query primary transport modes from primary transport */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_transports_packet(zse, buffer, buffer_out, &buffer_size);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_LIST, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 6, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	while (i < ARRAY_SIZE(transports)) {
		if (transports[i].found == true) {
			++entries;
		}

		++i;
	}

	zassert_equal(entries, 2, "Expected number of entries mismatch'");

	zassert_equal(transports[0].transport, SMP_SERIAL_TRANSPORT,
		      "Expected list 'id' mismatch");
	zassert_equal(transports[0].name.len, strlen("Dummy"),
		      "Expected list 'name' length mismatch");
	zassert_mem_equal(transports[0].name.value, "Dummy", transports[0].name.len,
			  "Expected list 'name' mismatch");

	zassert_equal(transports[1].transport, SMP_RAW_SERIAL_TRANSPORT,
		      "Expected list 'id' mismatch");
	zassert_equal(transports[1].name.len, strlen("Raw dummy"),
		      "Expected list 'name' length mismatch");
	zassert_mem_equal(transports[1].name.value, "Raw dummy", transports[1].name.len,
			  "Expected list 'name' mismatch");
}

ZTEST(transport_mgmt, test_valid_config_details)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct smp_hdr *header;
	size_t decoded = 0;
	struct transport_mgmt_configs configs[3] = { 0x0 };
	int rc;
	uint8_t i = 0;
	uint8_t entries = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("configs", transport_mgmt_config_decode, &configs),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	/* Test 1: Query dummy serial transport */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_config_details_packet(zse, buffer, buffer_out, &buffer_size,
							 SMP_SERIAL_TRANSPORT, 0);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_GET_CONFIG_DETAILS,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 6, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	/* Whilst "not successful", it is but it has an empty map as there is nothing to config */
	zassert_equal(rc, 0, "Expected decode to not be successful");
	zassert_equal(decoded, 0, "Expected to receive 1 decoded zcbor element");

	while (i < ARRAY_SIZE(configs)) {
		if (configs[i].found == true) {
			++entries;
		}

		++i;
	}

	zassert_equal(entries, 0, "Expected number of entries mismatch'");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	memset(&configs, 0, sizeof(configs));
	entries = 0;
	cleanup_test(NULL);

	/* Test 2: Query dummy raw serial transport */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_config_details_packet(zse, buffer, buffer_out, &buffer_size,
							 SMP_RAW_SERIAL_TRANSPORT, 0);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_GET_CONFIG_DETAILS,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 6, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	i = 0;
	while (i < ARRAY_SIZE(configs)) {
		if (configs[i].found == true) {
			++entries;
		}

		++i;
	}

	zassert_equal(entries, 2, "Expected number of entries mismatch'");

	zassert_equal(configs[0].name.len, strlen("port"),
		      "Expected config details 'name' length mismatch");
	zassert_mem_equal(configs[0].name.value, "port", configs[0].name.len,
			  "Expected config details 'name' mismatch");
	zassert_equal(configs[0].type, TRANSPORT_MGMT_CONFIG_TYPE_STRING,
		      "Expected config details 'type' mismatch");

	zassert_equal(configs[1].name.len, strlen("speed"),
		      "Expected config details 'name' length mismatch");
	zassert_mem_equal(configs[1].name.value, "speed", configs[1].name.len,
			  "Expected config details 'name' mismatch");
	zassert_equal(configs[1].type, TRANSPORT_MGMT_CONFIG_TYPE_UINT,
		      "Expected config details 'type' mismatch");
}

ZTEST(transport_mgmt, test_invalid_modes)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct smp_hdr *header;
	size_t decoded = 0;
	struct group_error_t group_error;
	int rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("err", mcumgr_ret_decode, &group_error),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	/* Test 1: Query UDP IPv6 (which is not enabled) */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_modes_packet(zse, buffer, buffer_out, &buffer_size,
						SMP_UDP_IPV6_TRANSPORT);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_GET_MODES,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 6, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(group_error.group, MGMT_GROUP_ID_TRANSPORT,
		      "Expected 'err' -> 'group' to be transport");
	zassert_equal(group_error.rc, TRANSPORT_MGMT_ERR_INVALID_TRANSPORT,
		      "Expected 'err' -> 'rc' to be invalid transport");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	memset(&group_error, 0, sizeof(group_error));
	cleanup_test(NULL);

	/* Test 2: Query small invalid ID */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_modes_packet(zse, buffer, buffer_out, &buffer_size, 13);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_GET_MODES,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 6, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(group_error.group, MGMT_GROUP_ID_TRANSPORT,
		      "Expected 'err' -> 'group' to be transport");
	zassert_equal(group_error.rc, TRANSPORT_MGMT_ERR_INVALID_TRANSPORT,
		      "Expected 'err' -> 'rc' to be invalid transport");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	memset(&group_error, 0, sizeof(group_error));
	cleanup_test(NULL);

	/* Test 3: Query large invalid ID */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_modes_packet(zse, buffer, buffer_out, &buffer_size, 165537);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_GET_MODES,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 6, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(group_error.group, MGMT_GROUP_ID_TRANSPORT,
		      "Expected 'err' -> 'group' to be transport");
	zassert_equal(group_error.rc, TRANSPORT_MGMT_ERR_INVALID_TRANSPORT,
		      "Expected 'err' -> 'rc' to be invalid transport");
}

ZTEST(transport_mgmt, test_invalid_config_details)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct smp_hdr *header;
	size_t decoded = 0;
	struct group_error_t group_error;
	int rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("err", mcumgr_ret_decode, &group_error),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	/* Test 1: Query invalid transport */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_config_details_packet(zse, buffer, buffer_out, &buffer_size,
							 SMP_UDP_IPV6_TRANSPORT, 0);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_GET_CONFIG_DETAILS,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 6, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(group_error.group, MGMT_GROUP_ID_TRANSPORT,
		      "Expected 'err' -> 'group' to be transport");
	zassert_equal(group_error.rc, TRANSPORT_MGMT_ERR_INVALID_TRANSPORT,
		      "Expected 'err' -> 'rc' to be invalid transport");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	cleanup_test(NULL);

	/* Test 2: Query valid transport (dummy) but invalid mode */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_config_details_packet(zse, buffer, buffer_out, &buffer_size,
							 SMP_SERIAL_TRANSPORT, 1);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_GET_CONFIG_DETAILS,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 6, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(group_error.group, MGMT_GROUP_ID_TRANSPORT,
		      "Expected 'err' -> 'group' to be transport");
	zassert_equal(group_error.rc, TRANSPORT_MGMT_ERR_INVALID_MODE,
		      "Expected 'err' -> 'rc' to be invalid transport");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	cleanup_test(NULL);

	/* Test 3: Query valid transport (raw dummy) but invalid mode */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_config_details_packet(zse, buffer, buffer_out, &buffer_size,
							 SMP_RAW_SERIAL_TRANSPORT, 2);
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

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_GET_CONFIG_DETAILS,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 6, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_equal(rc, 1, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(group_error.group, MGMT_GROUP_ID_TRANSPORT,
		      "Expected 'err' -> 'group' to be transport");
	zassert_equal(group_error.rc, TRANSPORT_MGMT_ERR_INVALID_MODE,
		      "Expected 'err' -> 'rc' to be invalid transport");
}

ZTEST_SUITE(transport_mgmt, NULL, NULL, NULL, cleanup_test, NULL);
