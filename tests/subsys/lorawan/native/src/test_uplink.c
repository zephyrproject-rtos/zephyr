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

ZTEST(lorawan_native_uplink, test_send_unconfirmed)
{
	prepare_send_test();
	network_server_prepare_uplink_no_response(LORAWAN_MSG_UNCONFIRMED, APP_PORT, app_payload,
						  sizeof(app_payload));

	zassert_ok(
		lorawan_send(APP_PORT, app_payload, sizeof(app_payload), LORAWAN_MSG_UNCONFIRMED));
	zassert_equal(fake_lora_send_async_fake.call_count, 1U);
	zassert_equal(fake_lora_recv_fake.call_count, 2U);
	zassert_equal(fake_lora_config_fake.call_count, 3U);

	/* The server fixture expects FCntUp=1 and derives its MIC and ciphertext from it. */
	network_server_prepare_uplink_no_response(LORAWAN_MSG_UNCONFIRMED, APP_PORT, app_payload,
						  sizeof(app_payload));
	zassert_ok(
		lorawan_send(APP_PORT, app_payload, sizeof(app_payload), LORAWAN_MSG_UNCONFIRMED));
	zassert_equal(fake_lora_send_async_fake.call_count, 1U);
	zassert_equal(fake_lora_recv_fake.call_count, 2U);
	zassert_equal(fake_lora_config_fake.call_count, 3U);
}

ZTEST(lorawan_native_uplink, test_send_invalid_message_type)
{
	prepare_send_test();

	zassert_equal(lorawan_send(APP_PORT, app_payload, sizeof(app_payload),
				   (enum lorawan_message_type)2),
		      -EINVAL);
	zassert_equal(fake_lora_send_async_fake.call_count, 1U);
}

ZTEST(lorawan_native_uplink, test_api_returns_error_before_join)
{
	zassert_equal(
		lorawan_send(APP_PORT, app_payload, sizeof(app_payload), LORAWAN_MSG_UNCONFIRMED),
		-ENETDOWN);
	zassert_equal(lorawan_request_link_check(true), -ENETDOWN);
}

ZTEST(lorawan_native_uplink, test_set_datarate_before_join)
{
	enum lorawan_datarate datarate = test_region_get()->test_datarate;

	zassert_ok(lorawan_set_datarate(datarate));
	run_successful_join(1U);

	network_server_prepare_uplink_no_response(LORAWAN_MSG_UNCONFIRMED, APP_PORT, app_payload,
						  sizeof(app_payload));
	network_server_expect_uplink_datarate(datarate);

	zassert_ok(
		lorawan_send(APP_PORT, app_payload, sizeof(app_payload), LORAWAN_MSG_UNCONFIRMED));
}

ZTEST(lorawan_native_uplink, test_set_datarate_after_join)
{
	enum lorawan_datarate datarate = test_region_get()->test_datarate;

	run_successful_join(1U);
	zassert_ok(lorawan_set_datarate(datarate));

	network_server_prepare_uplink_no_response(LORAWAN_MSG_UNCONFIRMED, APP_PORT, app_payload,
						  sizeof(app_payload));
	network_server_expect_uplink_datarate(datarate);

	zassert_ok(
		lorawan_send(APP_PORT, app_payload, sizeof(app_payload), LORAWAN_MSG_UNCONFIRMED));
}

ZTEST(lorawan_native_uplink, test_send_confirmed_acked)
{
	prepare_send_test();
	network_server_prepare_ack(1U, APP_PORT, app_payload, sizeof(app_payload));

	zassert_ok(lorawan_send(APP_PORT, app_payload, sizeof(app_payload), LORAWAN_MSG_CONFIRMED));
	zassert_equal(fake_lora_send_async_fake.call_count, 1U);
	zassert_equal(fake_lora_recv_fake.call_count, 1U);
	zassert_equal(fake_lora_config_fake.call_count, 2U);
}

ZTEST(lorawan_native_uplink, test_send_confirmed_not_acked)
{
	prepare_send_test();
	network_server_prepare_uplink_no_response(LORAWAN_MSG_CONFIRMED, APP_PORT, app_payload,
						  sizeof(app_payload));

	zassert_equal(
		lorawan_send(APP_PORT, app_payload, sizeof(app_payload), LORAWAN_MSG_CONFIRMED),
		-ETIMEDOUT);
	zassert_equal(fake_lora_send_async_fake.call_count, 1U);
	zassert_equal(fake_lora_recv_fake.call_count, 2U);
	zassert_equal(fake_lora_config_fake.call_count, 3U);
}

ZTEST(lorawan_native_uplink, test_send_confirmed_two_tries_acked_on_first)
{
	prepare_send_test();
	zassert_ok(lorawan_set_conf_msg_tries(2U));
	network_server_prepare_ack(1U, APP_PORT, app_payload, sizeof(app_payload));

	zassert_ok(lorawan_send(APP_PORT, app_payload, sizeof(app_payload), LORAWAN_MSG_CONFIRMED));
	zassert_equal(fake_lora_send_async_fake.call_count, 1U);
	zassert_equal(fake_lora_recv_fake.call_count, 1U);
	zassert_equal(fake_lora_config_fake.call_count, 2U);
}

ZTEST(lorawan_native_uplink, test_send_confirmed_two_tries_acked_on_second)
{
	prepare_send_test();
	zassert_ok(lorawan_set_conf_msg_tries(2U));
	network_server_prepare_ack(2U, APP_PORT, app_payload, sizeof(app_payload));

	zassert_ok(lorawan_send(APP_PORT, app_payload, sizeof(app_payload), LORAWAN_MSG_CONFIRMED));
	zassert_equal(fake_lora_send_async_fake.call_count, 2U);
	zassert_equal(fake_lora_recv_fake.call_count, 3U);
	zassert_equal(fake_lora_config_fake.call_count, 5U);
}

ZTEST(lorawan_native_uplink, test_send_confirmed_two_tries_not_acked)
{
	prepare_send_test();
	zassert_ok(lorawan_set_conf_msg_tries(2U));
	network_server_prepare_uplink_no_response(LORAWAN_MSG_CONFIRMED, APP_PORT, app_payload,
						  sizeof(app_payload));

	zassert_equal(
		lorawan_send(APP_PORT, app_payload, sizeof(app_payload), LORAWAN_MSG_CONFIRMED),
		-ETIMEDOUT);
	zassert_equal(fake_lora_send_async_fake.call_count, 2U);
	zassert_equal(fake_lora_recv_fake.call_count, 4U);
	zassert_equal(fake_lora_config_fake.call_count, 6U);
}

ZTEST(lorawan_native_uplink, test_api_returns_error_before_start)
{
	uint8_t dev_eui[8] = {0};
	uint8_t join_eui[8] = {0};
	uint8_t nwk_key[16] = {0};
	struct lorawan_join_config join_config = {
		.mode = LORAWAN_ACT_OTAA,
		.dev_eui = dev_eui,
		.otaa = {.join_eui = join_eui, .nwk_key = nwk_key},
	};
	uint16_t channels_mask = UINT16_MAX;

	lorawan_stop();
	zassert_equal(lorawan_join(&join_config), -EPERM);
	zassert_equal(lorawan_send(APP_PORT, NULL, 0U, LORAWAN_MSG_UNCONFIRMED), -EPERM);
	zassert_equal(lorawan_set_datarate(LORAWAN_DR_5), -EPERM);
	zassert_equal(lorawan_set_conf_msg_tries(1U), -EPERM);
	zassert_equal(lorawan_set_channels_mask(&channels_mask, 1U), -EPERM);
	zassert_equal(lorawan_request_link_check(false), -EPERM);
}

ZTEST_SUITE(lorawan_native_uplink, NULL, NULL, lorawan_native_before, lorawan_native_after, NULL);
