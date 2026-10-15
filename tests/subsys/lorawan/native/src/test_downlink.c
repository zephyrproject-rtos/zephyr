/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network_server.h"
#include "test_common.h"

#include <string.h>

#include <zephyr/drivers/lora/lora_fake.h>
#include <zephyr/kernel.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/ztest.h>

static K_SEM_DEFINE(downlink_received, 0, 1);
static uint8_t received_downlink[sizeof(app_payload)];
static uint8_t received_downlink_port;
static uint8_t received_downlink_flags;
static int16_t received_downlink_rssi;
static int8_t received_downlink_snr;
static uint8_t received_downlink_len;

static void application_downlink_callback(uint8_t port, uint8_t flags, int16_t rssi, int8_t snr,
					  uint8_t len, const uint8_t *data)
{
	received_downlink_port = port;
	received_downlink_flags = flags;
	received_downlink_rssi = rssi;
	received_downlink_snr = snr;
	received_downlink_len = len;
	if (len > 0U) {
		memcpy(received_downlink, data, len);
	}
	k_sem_give(&downlink_received);
}

static struct lorawan_downlink_cb application_downlink_cb = {
	.port = APP_PORT,
	.cb = application_downlink_callback,
};

static void prepare_application_downlink_callback(void)
{
	k_sem_reset(&downlink_received);
	received_downlink_len = 0U;
	lorawan_register_downlink_callback(&application_downlink_cb);
}

static void assert_application_downlink_received(uint8_t expected_flags)
{
	zassert_ok(k_sem_take(&downlink_received, K_SECONDS(1)));
	zassert_equal(received_downlink_port, APP_PORT);
	zassert_equal(received_downlink_flags, expected_flags);
	zassert_equal(received_downlink_rssi, -42);
	zassert_equal(received_downlink_snr, 7);
	zassert_equal(received_downlink_len, sizeof(app_payload));
	zassert_mem_equal(received_downlink, app_payload, sizeof(app_payload));
}

ZTEST(lorawan_native_downlink, test_receive_application_rx1)
{
	prepare_send_test();
	prepare_application_downlink_callback();
	network_server_prepare_downlink(1U, APP_PORT, app_payload, sizeof(app_payload));

	zassert_ok(
		lorawan_send(APP_PORT, app_payload, sizeof(app_payload), LORAWAN_MSG_UNCONFIRMED));
	assert_application_downlink_received(0U);
	zassert_equal(fake_lora_recv_fake.call_count, 1U);
}

ZTEST(lorawan_native_downlink, test_receive_application_rx2)
{
	prepare_send_test();
	prepare_application_downlink_callback();
	network_server_prepare_downlink(2U, APP_PORT, app_payload, sizeof(app_payload));

	zassert_ok(
		lorawan_send(APP_PORT, app_payload, sizeof(app_payload), LORAWAN_MSG_UNCONFIRMED));
	assert_application_downlink_received(0U);
	zassert_equal(fake_lora_recv_fake.call_count, 2U);
}

ZTEST(lorawan_native_downlink, test_receive_application_data_pending)
{
	prepare_send_test();
	prepare_application_downlink_callback();
	network_server_prepare_data_pending_downlink(1U, APP_PORT, app_payload,
						    sizeof(app_payload));

	zassert_ok(
		lorawan_send(APP_PORT, app_payload, sizeof(app_payload), LORAWAN_MSG_UNCONFIRMED));
	assert_application_downlink_received(LORAWAN_DATA_PENDING);
}

ZTEST(lorawan_native_downlink, test_acknowledge_confirmed_downlink)
{
	prepare_send_test();
	prepare_application_downlink_callback();
	network_server_prepare_confirmed_downlink(1U, APP_PORT, app_payload, sizeof(app_payload));

	zassert_ok(
		lorawan_send(APP_PORT, app_payload, sizeof(app_payload), LORAWAN_MSG_UNCONFIRMED));
	assert_application_downlink_received(0U);

	/* A confirmed server downlink must set ACK on the next uplink. */
	network_server_prepare_uplink_ack(APP_PORT, app_payload, sizeof(app_payload));
	zassert_ok(
		lorawan_send(APP_PORT, app_payload, sizeof(app_payload), LORAWAN_MSG_UNCONFIRMED));
	zassert_equal(fake_lora_send_async_fake.call_count, 1U);
	zassert_equal(fake_lora_recv_fake.call_count, 2U);
}

ZTEST(lorawan_native_downlink, test_invalid_application_downlinks_rejected)
{
	static const enum network_server_invalid_downlink invalid_downlinks[] = {
		NETWORK_SERVER_INVALID_DOWNLINK_WRONG_MHDR,
		NETWORK_SERVER_INVALID_DOWNLINK_UNSUPPORTED_MAJOR,
		NETWORK_SERVER_INVALID_DOWNLINK_SHORT,
		NETWORK_SERVER_INVALID_DOWNLINK_WRONG_DEV_ADDR,
		NETWORK_SERVER_INVALID_DOWNLINK_BAD_MIC,
		NETWORK_SERVER_INVALID_DOWNLINK_BAD_FOPTS_LEN,
	};

	prepare_send_test();
	prepare_application_downlink_callback();
	for (size_t i = 0U; i < ARRAY_SIZE(invalid_downlinks); i++) {
		k_sem_reset(&downlink_received);
		network_server_prepare_invalid_downlink(invalid_downlinks[i], APP_PORT, app_payload,
							sizeof(app_payload));

		zassert_ok(lorawan_send(APP_PORT, app_payload, sizeof(app_payload),
					LORAWAN_MSG_UNCONFIRMED));
		zassert_equal(k_sem_take(&downlink_received, K_MSEC(50)), -EAGAIN);
		zassert_equal(fake_lora_recv_fake.call_count, 2U);
	}
}

ZTEST_SUITE(lorawan_native_downlink, NULL, NULL, lorawan_native_before, lorawan_native_after, NULL);
