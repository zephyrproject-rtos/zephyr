/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network_server.h"
#include "test_common.h"
#include "region/test_region.h"

#include <zephyr/drivers/lora/lora_fake.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/ztest.h>

static uint8_t vector_app_payload[] = {
	0x81, 0x05, 0x00, 0x0a, 0x14, 0x03, 0x3e, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x0e, 0x7e, 0x09, 0x42, 0x0a, 0x8c, 0x00, 0x00,
};

static void
assert_invalid_join_response_rejected(enum network_server_invalid_join_response response)
{
	struct lorawan_join_config join_config = network_server_join_config();

	network_server_prepare_invalid_join_responses(response);
	zassert_equal(lorawan_join(&join_config), -ETIMEDOUT);
	zassert_equal(fake_lora_send_async_fake.call_count, 1U);
	zassert_equal(fake_lora_recv_fake.call_count, 2U);
	zassert_equal(fake_lora_config_fake.call_count, 3U);
}

ZTEST(lorawan_native_join, test_accept_rx1)
{
	run_successful_join(1U);
}

ZTEST(lorawan_native_join, test_accept_rx2)
{
	run_successful_join(2U);
}

ZTEST(lorawan_native_join, test_accept_rx_delay)
{
	struct lorawan_join_config join_config = network_server_join_config();

	network_server_prepare_join_with_rx_delay(3U);
	zassert_ok(lorawan_join(&join_config));

	network_server_prepare_uplink_no_response(LORAWAN_MSG_UNCONFIRMED, APP_PORT, app_payload,
						  sizeof(app_payload));
	network_server_expect_rx_delay(3U);
	zassert_ok(
		lorawan_send(APP_PORT, app_payload, sizeof(app_payload), LORAWAN_MSG_UNCONFIRMED));
}

ZTEST(lorawan_native_join, test_lorawan_1_0_x_vector)
{
	struct lorawan_join_config join_config = network_server_vector_join_config();

	if (!test_region_get()->has_1_0_x_vector) {
		ztest_test_skip();
	}

	lorawan_enable_adr(true);
	network_server_prepare_vector_join();
	zassert_ok(lorawan_join(&join_config));
	zassert_equal(fake_lora_send_async_fake.call_count, 1U);
	zassert_equal(fake_lora_recv_fake.call_count, 1U);

	/* The hardcoded uplink verifies the derived AppSKey and FNwkSIntKey. */
	network_server_prepare_vector_uplink();
	zassert_ok(lorawan_send(2U, vector_app_payload, sizeof(vector_app_payload),
				LORAWAN_MSG_UNCONFIRMED));

	lorawan_enable_adr(false);
}

ZTEST(lorawan_native_join, test_invalid_response)
{
	struct lorawan_join_config join_config = network_server_join_config();

	network_server_prepare_invalid_join_response();
	zassert_ok(lorawan_join(&join_config));

	zassert_equal(fake_lora_send_async_fake.call_count, 1U);
	zassert_equal(fake_lora_recv_fake.call_count, 2U);
	zassert_equal(fake_lora_config_fake.call_count, 3U);
}

ZTEST(lorawan_native_join, test_invalid_responses_rejected)
{
	static const enum network_server_invalid_join_response invalid_responses[] = {
		NETWORK_SERVER_INVALID_JOIN_RESPONSE_WRONG_MHDR,
		NETWORK_SERVER_INVALID_JOIN_RESPONSE_UNSUPPORTED_MAJOR,
		NETWORK_SERVER_INVALID_JOIN_RESPONSE_EMPTY,
		NETWORK_SERVER_INVALID_JOIN_RESPONSE_SHORT,
		NETWORK_SERVER_INVALID_JOIN_RESPONSE_INTERMEDIATE,
		NETWORK_SERVER_INVALID_JOIN_RESPONSE_OVERSIZE,
		NETWORK_SERVER_INVALID_JOIN_RESPONSE_BAD_CIPHERTEXT,
		NETWORK_SERVER_INVALID_JOIN_RESPONSE_BAD_MIC,
		NETWORK_SERVER_INVALID_JOIN_RESPONSE_INVALID_RX1_OFFSET,
		NETWORK_SERVER_INVALID_JOIN_RESPONSE_INVALID_RX2_DATARATE,
	};

	for (size_t i = 0U; i < ARRAY_SIZE(invalid_responses); i++) {
		assert_invalid_join_response_rejected(invalid_responses[i]);
	}
}

ZTEST(lorawan_native_join, test_accept_with_cflist)
{
	struct lorawan_join_config join_config = network_server_join_config();
	const struct test_region *region = test_region_get();
	uint16_t channel_mask = BIT(region->cflist_first_channel_idx);

	if (region->cflist_frequency_count == 0U) {
		ztest_test_skip();
	}

	network_server_prepare_join_with_cflist(1U);
	zassert_ok(lorawan_join(&join_config));

	zassert_equal(fake_lora_send_async_fake.call_count, 1U);
	zassert_equal(fake_lora_recv_fake.call_count, 1U);
	zassert_equal(fake_lora_config_fake.call_count, 2U);

	/* Request stack to use the first CFLIST channel */
	zassert_ok(lorawan_set_channels_mask(&channel_mask, 1U));
	network_server_prepare_uplink_no_response(LORAWAN_MSG_UNCONFIRMED, APP_PORT, app_payload,
						  sizeof(app_payload));
	network_server_expect_tx_frequency(region->cflist_frequencies[0]);

	zassert_ok(
		lorawan_send(APP_PORT, app_payload, sizeof(app_payload), LORAWAN_MSG_UNCONFIRMED));

	zassert_equal(fake_lora_send_async_fake.call_count, 1U);

	channel_mask = UINT16_MAX;
	zassert_ok(lorawan_set_channels_mask(&channel_mask, 1U));
}

ZTEST(lorawan_native_join, test_no_response)
{
	struct lorawan_join_config join_config = network_server_join_config();

	network_server_prepare_join_no_response();
	zassert_equal(lorawan_join(&join_config), -ETIMEDOUT);
	zassert_equal(fake_lora_send_async_fake.call_count, 1U);
	zassert_equal(fake_lora_recv_fake.call_count, 2U);
}

ZTEST_SUITE(lorawan_native_join, NULL, NULL, lorawan_native_before, lorawan_native_after, NULL);
