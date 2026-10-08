/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/transport_mgmt/transport_mgmt.h>
#include <mgmt/mcumgr/transport/smp_internal.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include "smp_test_util.h"

#define ZCBOR_STATES 4

/* Starts encoding the payload of a packet, after the space left for its header */
static void start_payload(zcbor_state_t *zse, uint8_t *buffer, size_t size)
{
	zassert_true(size > sizeof(struct smp_hdr), "Packet buffer too small");
	zcbor_new_encode_state(zse, ZCBOR_STATES, &buffer[sizeof(struct smp_hdr)],
			       size - sizeof(struct smp_hdr), 0);
}

/* Writes the header of a packet whose payload is encoded, and returns the packet length */
static size_t finish_packet(uint8_t *buffer, const zcbor_state_t *zse, uint8_t op, uint16_t group,
			    uint8_t id, uint8_t seq)
{
	size_t payload_len = zse->payload_mut - &buffer[sizeof(struct smp_hdr)];
	struct smp_hdr hdr = {
		.nh_op = op,
		.nh_version = 1,
		.nh_flags = 0,
		.nh_len = sys_cpu_to_be16(payload_len),
		.nh_group = sys_cpu_to_be16(group),
		.nh_seq = seq,
		.nh_id = id,
	};

	memcpy(buffer, &hdr, sizeof(hdr));

	return sizeof(hdr) + payload_len;
}

static size_t build_echo(uint8_t *buffer, size_t size, const char *key, const char *data,
			 uint8_t op, uint8_t seq)
{
	zcbor_state_t zse[ZCBOR_STATES];
	bool ok;

	start_payload(zse, buffer, size);
	ok = zcbor_map_start_encode(zse, 1) &&
	     zcbor_tstr_put_term(zse, key, CONFIG_ZCBOR_MAX_STR_LEN) &&
	     zcbor_tstr_put_term(zse, data, CONFIG_ZCBOR_MAX_STR_LEN) &&
	     zcbor_map_end_encode(zse, 1);
	zassert_true(ok, "Failed to encode echo packet");

	return finish_packet(buffer, zse, op, MGMT_GROUP_ID_OS, OS_MGMT_ID_ECHO, seq);
}

size_t smp_test_build_echo_request(uint8_t *buffer, size_t size, const char *data, uint8_t seq)
{
	return build_echo(buffer, size, "d", data, MGMT_OP_READ, seq);
}

size_t smp_test_build_echo_response(uint8_t *buffer, size_t size, const char *data, uint8_t seq)
{
	return build_echo(buffer, size, "r", data, MGMT_OP_READ_RSP, seq);
}

size_t smp_test_build_connect_request(uint8_t *buffer, size_t size, uint32_t transport, uint8_t seq)
{
	zcbor_state_t zse[ZCBOR_STATES];
	bool ok;

	start_payload(zse, buffer, size);
	ok = zcbor_map_start_encode(zse, 1) && zcbor_tstr_put_lit(zse, "transport") &&
	     zcbor_uint32_put(zse, transport) && zcbor_map_end_encode(zse, 1);
	zassert_true(ok, "Failed to encode connect request");

	return finish_packet(buffer, zse, MGMT_OP_WRITE, MGMT_GROUP_ID_TRANSPORT,
			     TRANSPORT_MGMT_ID_CONNECT, seq);
}

void smp_test_check_echo_response(const uint8_t *packet, size_t len, const char *data, uint8_t seq)
{
	struct smp_hdr hdr;
	zcbor_state_t zsd[ZCBOR_STATES];
	struct zcbor_string echoed = {0};
	size_t decoded = 0;
	int rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("r", zcbor_tstr_decode, &echoed),
	};

	zassert_true(len >= sizeof(hdr), "Response is shorter than an SMP header");
	memcpy(&hdr, packet, sizeof(hdr));

	zassert_equal(hdr.nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(sys_be16_to_cpu(hdr.nh_group), MGMT_GROUP_ID_OS, "SMP header group mismatch");
	zassert_equal(hdr.nh_id, OS_MGMT_ID_ECHO, "SMP header command ID mismatch");
	zassert_equal(hdr.nh_seq, seq, "SMP header sequence number mismatch");
	zassert_equal(sys_be16_to_cpu(hdr.nh_len), len - sizeof(hdr), "SMP header length mismatch");

	zcbor_new_decode_state(zsd, ARRAY_SIZE(zsd), &packet[sizeof(hdr)], len - sizeof(hdr), 1,
			       NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded);
	zassert_equal(rc, 0, "Failed to decode echo response");
	zassert_equal(decoded, 1, "Expected 1 decoded element");
	zassert_equal(echoed.len, strlen(data), "Echo response length mismatch");
	zassert_mem_equal(echoed.value, data, echoed.len, "Echo response mismatch");
}
