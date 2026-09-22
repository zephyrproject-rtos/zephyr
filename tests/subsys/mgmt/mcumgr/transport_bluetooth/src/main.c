/*
 * Copyright (c) 2026 Jamie McCrae
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp_dummy.h>
#include <zephyr/mgmt/mcumgr/grp/transport_mgmt/transport_mgmt.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/transport/smp_internal.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>

#if !defined(CONFIG_MCUMGR_GRP_TRANSPORT)
#error "Expected Kconfig option CONFIG_MCUMGR_GRP_TRANSPORT not enabled"
#endif

#if !defined(CONFIG_MCUMGR_TRANSPORT_BT)
#error "Expected Kconfig option CONFIG_MCUMGR_TRANSPORT_BT not enabled"
#endif

#define SMP_RESPONSE_WAIT_TIME 3
#define REQUEST_PAYLOAD_SIZE 32
#define ZCBOR_HISTORY_ARRAY_SIZE 8

struct transport_mgmt_config {
	struct zcbor_string name;
	uint32_t type;
	bool required;
};

static struct net_buf *nb;

static void cleanup_test(void *p)
{
	ARG_UNUSED(p);

	if (nb != NULL) {
		net_buf_reset(nb);
		net_buf_unref(nb);
		nb = NULL;
	}
}

static void transport_mgmt_query(uint8_t command_id, const uint8_t *payload, size_t payload_len)
{
	const struct smp_hdr request_header = {
		.nh_op = MGMT_OP_READ,
		.nh_version = 1,
		.nh_len = sys_cpu_to_be16(payload_len),
		.nh_group = sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		.nh_seq = 1,
		.nh_id = command_id,
	};
	uint8_t request[sizeof(request_header) + REQUEST_PAYLOAD_SIZE];
	struct smp_hdr *response_header;

	memcpy(request, &request_header, sizeof(request_header));
	memcpy(&request[sizeof(request_header)], payload, payload_len);

	smp_dummy_enable();
	smp_dummy_clear_state();
	(void)smp_dummy_tx_pkt(request, sizeof(request_header) + payload_len);
	smp_dummy_add_data();

	zassert_true(smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME),
		     "Expected to receive data but timed out");

	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();
	zassert_not_null(nb, "Expected a complete SMP response");

	response_header = net_buf_pull_mem(nb, sizeof(*response_header));

	zassert_equal(response_header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(response_header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(response_header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "SMP header group mismatch");
	zassert_equal(response_header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(response_header->nh_id, command_id, "SMP header command ID mismatch");
	zassert_equal(response_header->nh_version, 1, "SMP header version mismatch");
}

static void transport_mgmt_modes_query(uint32_t transport)
{
	uint8_t payload[REQUEST_PAYLOAD_SIZE];
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE];
	bool ok;

	zcbor_new_encode_state(zse, ARRAY_SIZE(zse), payload, sizeof(payload), 0);
	ok = zcbor_map_start_encode(zse, 1) &&
	     zcbor_tstr_put_lit(zse, "transport") &&
	     zcbor_uint32_put(zse, transport) &&
	     zcbor_map_end_encode(zse, 1);
	zassert_true(ok, "Expected packet creation to be successful");

	transport_mgmt_query(TRANSPORT_MGMT_ID_GET_MODES, payload, zse->payload_mut - payload);
}

static void transport_mgmt_config_details_query(uint32_t transport, uint32_t mode)
{
	uint8_t payload[REQUEST_PAYLOAD_SIZE];
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE];
	bool ok;

	zcbor_new_encode_state(zse, ARRAY_SIZE(zse), payload, sizeof(payload), 0);
	ok = zcbor_map_start_encode(zse, 2) &&
	     zcbor_tstr_put_lit(zse, "transport") &&
	     zcbor_uint32_put(zse, transport) &&
	     zcbor_tstr_put_lit(zse, "mode") &&
	     zcbor_uint32_put(zse, mode) &&
	     zcbor_map_end_encode(zse, 2);
	zassert_true(ok, "Expected packet creation to be successful");

	transport_mgmt_query(TRANSPORT_MGMT_ID_GET_CONFIG_DETAILS, payload,
			     zse->payload_mut - payload);
}

ZTEST(transport_bluetooth, test_modes)
{
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE];
	uint32_t id = UINT32_MAX;
	struct zcbor_string description = { 0 };
	bool incoming = false;
	bool outgoing = false;
	size_t decoded = 0;
	bool ok;

	struct zcbor_map_decode_key_val mode_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("id", zcbor_uint32_decode, &id),
		ZCBOR_MAP_DECODE_KEY_DECODER("description", zcbor_tstr_decode, &description),
		ZCBOR_MAP_DECODE_KEY_DECODER("incoming", zcbor_bool_decode, &incoming),
		ZCBOR_MAP_DECODE_KEY_DECODER("outgoing", zcbor_bool_decode, &outgoing),
	};

	transport_mgmt_modes_query(SMP_BLUETOOTH_TRANSPORT);

	zcbor_new_decode_state(zsd, ARRAY_SIZE(zsd), nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_start_decode(zsd) &&
	     zcbor_tstr_expect_lit(zsd, "modes") &&
	     zcbor_list_start_decode(zsd) &&
	     zcbor_map_decode_bulk(zsd, mode_decode, ARRAY_SIZE(mode_decode), &decoded) == 0 &&
	     zcbor_list_end_decode(zsd) &&
	     zcbor_map_end_decode(zsd);
	zassert_true(ok, "Expected decode of a single transport mode to be successful");

	zassert_true(zcbor_map_decode_bulk_key_found(mode_decode, ARRAY_SIZE(mode_decode), "id"),
		     "Expected mode 'id' to be present");
	zassert_equal(decoded, ARRAY_SIZE(mode_decode), "Expected all mode fields to be present");
	zassert_equal(id, 0, "Expected mode 'id' mismatch");
	zassert_equal(description.len, strlen("Bluetooth Low Energy"),
		      "Expected mode 'description' length mismatch");
	zassert_mem_equal(description.value, "Bluetooth Low Energy", description.len,
			  "Expected mode 'description' mismatch");
	zassert_true(incoming, "Expected mode 'incoming' mismatch");
	zassert_true(outgoing, "Expected mode 'outgoing' mismatch");
}

ZTEST(transport_bluetooth, test_config_details)
{
	static const struct {
		const char *name;
		uint32_t type;
	} expected[] = {
		{ .name = "address_type", .type = TRANSPORT_MGMT_CONFIG_TYPE_UINT },
		{ .name = "address", .type = TRANSPORT_MGMT_CONFIG_TYPE_STRING },
		{ .name = "le_coded", .type = TRANSPORT_MGMT_CONFIG_TYPE_BOOL },
	};
	struct transport_mgmt_config configs[ARRAY_SIZE(expected)] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE];
	bool ok;

	transport_mgmt_config_details_query(SMP_BLUETOOTH_TRANSPORT, 0);

	zcbor_new_decode_state(zsd, ARRAY_SIZE(zsd), nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_start_decode(zsd) &&
	     zcbor_tstr_expect_lit(zsd, "configs") &&
	     zcbor_list_start_decode(zsd);

	for (size_t i = 0; ok && i < ARRAY_SIZE(configs); ++i) {
		size_t decoded = 0;
		struct zcbor_map_decode_key_val config_decode[] = {
			ZCBOR_MAP_DECODE_KEY_DECODER("name", zcbor_tstr_decode, &configs[i].name),
			ZCBOR_MAP_DECODE_KEY_DECODER("type", zcbor_uint32_decode, &configs[i].type),
			ZCBOR_MAP_DECODE_KEY_DECODER("required", zcbor_bool_decode,
						     &configs[i].required),
		};

		ok = zcbor_map_decode_bulk(zsd, config_decode, ARRAY_SIZE(config_decode),
					   &decoded) == 0 &&
		     decoded == ARRAY_SIZE(config_decode);
	}

	ok = ok && zcbor_list_end_decode(zsd) && zcbor_map_end_decode(zsd);
	zassert_true(ok, "Expected decode of %zu configuration items to be successful",
		     ARRAY_SIZE(expected));

	for (size_t i = 0; i < ARRAY_SIZE(expected); ++i) {
		zassert_equal(configs[i].name.len, strlen(expected[i].name),
			      "Expected config %zu 'name' length mismatch", i);
		zassert_mem_equal(configs[i].name.value, expected[i].name, configs[i].name.len,
				  "Expected config %zu 'name' mismatch", i);
		zassert_equal(configs[i].type, expected[i].type,
			      "Expected config %zu 'type' mismatch", i);
		zassert_true(configs[i].required, "Expected config %zu 'required' mismatch", i);
	}
}

ZTEST(transport_bluetooth, test_config_details_invalid_mode)
{
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE];
	uint32_t group = 0;
	uint32_t rc = 0;
	size_t decoded = 0;
	bool ok;

	struct zcbor_map_decode_key_val error_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("group", zcbor_uint32_decode, &group),
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_uint32_decode, &rc),
	};

	transport_mgmt_config_details_query(SMP_BLUETOOTH_TRANSPORT, 1);

	zcbor_new_decode_state(zsd, ARRAY_SIZE(zsd), nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_start_decode(zsd) &&
	     zcbor_tstr_expect_lit(zsd, "err") &&
	     zcbor_map_decode_bulk(zsd, error_decode, ARRAY_SIZE(error_decode), &decoded) == 0 &&
	     zcbor_map_end_decode(zsd);
	zassert_true(ok, "Expected decode of a group error to be successful");

	zassert_equal(decoded, ARRAY_SIZE(error_decode), "Expected all error fields to be present");
	zassert_equal(group, MGMT_GROUP_ID_TRANSPORT, "Expected 'err' -> 'group' to be transport");
	zassert_equal(rc, TRANSPORT_MGMT_ERR_INVALID_MODE,
		      "Expected 'err' -> 'rc' to be invalid mode");
}

ZTEST_SUITE(transport_bluetooth, NULL, NULL, NULL, cleanup_test, NULL);
