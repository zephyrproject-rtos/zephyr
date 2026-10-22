/*
 * Copyright (c) 2023-2026 Nordic Semiconductor ASA
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
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include "smp_test_util.h"

#define SMP_RESPONSE_WAIT_TIME 3
#define ZCBOR_BUFFER_SIZE 128
#define OUTPUT_BUFFER_SIZE 384
#define ZCBOR_HISTORY_ARRAY_SIZE 8
#define TEST_STRING "tEsTiNg eChO dAtA"

static struct net_buf *nb;

static void cleanup_test(void *p)
{
	if (nb != NULL) {
		net_buf_reset(nb);
		net_buf_unref(nb);
		nb = NULL;
	}
}

ZTEST(transport_raw_uart, test_os_mgmt_echo)
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
	struct zcbor_string data = { 0 };

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("r", zcbor_tstr_decode, &data),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	/* Test 1: Get the unused default memory pool values as a baseline */
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_echo_packet(zse, buffer, buffer_out, &buffer_size, TEST_STRING);
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
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");
	zassert_equal(data.len, strlen(TEST_STRING), "os mgmt echo response length mismatch");
	zassert_mem_equal(data.value, TEST_STRING, data.len, "os mgmt echo response mismatch");
}

ZTEST_SUITE(transport_raw_uart, NULL, NULL, NULL, cleanup_test, NULL);
