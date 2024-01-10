/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_LIMITED_TEST)

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
#include <string.h>
#include <smp_internal.h>
#include "smp_test_util.h"

#define SMP_RESPONSE_WAIT_TIME 3
#define ZCBOR_BUFFER_SIZE 64
#define OUTPUT_BUFFER_SIZE 64
#define ZCBOR_HISTORY_ARRAY_SIZE 4

static struct net_buf *nb;

/* Responses to commands */
const uint8_t response_kernel_name[] = "Zephyr";

const uint8_t query_kernel_name[] = "s";
const uint8_t query_all[] = "a";

ZTEST(os_mgmt_info_limited, test_info_1_kernel_name)
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

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, query_kernel_name, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
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

	zassert_equal((sizeof(response_kernel_name) - 1), output.len,
		      "Expected to receive %d bytes but got %d\n",
		      (sizeof(response_kernel_name) - 1), output.len);

	zassert_mem_equal(response_kernel_name, output.value, output.len,
			  "Expected received data mismatch");
}

ZTEST(os_mgmt_info_limited, test_info_2_all)
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
	int32_t rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	struct zcbor_map_decode_key_val error_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &rc),
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

	/* Send query command to dummy SMP backend */
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

	/* Ensure only an error is received */
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 0, "Expected to receive 0 decoded zcbor element\n");

	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_decode_bulk(zsd, error_decode, ARRAY_SIZE(error_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element\n");
	zassert_equal(output.len, 0, "Expected to receive 0 bytes but got %d\n", output.len);
	zassert_equal(rc, MGMT_ERR_EMSGSIZE, "Expected to receive EMSGSIZE error but got %d\n",
		      rc);
}

static void cleanup_test(void *p)
{
	if (nb != NULL) {
		net_buf_unref(nb);
		nb = NULL;
	}
}

/* Limited size buffer test set */
ZTEST_SUITE(os_mgmt_info_limited, NULL, NULL, NULL, cleanup_test, NULL);

#endif
