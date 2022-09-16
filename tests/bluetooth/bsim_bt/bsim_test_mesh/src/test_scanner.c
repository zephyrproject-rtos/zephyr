/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include "mesh_test.h"
#include "mesh/net.h"
#include "mesh/adv.h"
#include "mesh/mesh.h"
#include "mesh/foundation.h"

#define LOG_MODULE_NAME test_scanner

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define WAIT_TIME 60 /*seconds*/
#define ADV_INT_FAST_MS 20
#define ADV_DURATION 120

extern enum bst_result_t bst_result;

/* Create a valid message */
static const uint8_t valid_message[28] = {
	0x0d, 0x10, 0xca, 0x54, 0xd0,
	0x00, 0x24, 0x00, 0xaa, 0x8c,
	0xcc, 0x6b, 0x6a, 0xc8, 0x51,
	0x69, 0x16, 0x4d, 0xf6, 0x9b,
	0xce, 0xbd, 0xc7, 0xa3, 0xf0,
	0x28, 0xdf, 0xae
};

static const struct bt_mesh_test_cfg tx_cfg = {
	.addr = 0x0001,
	.dev_key = { 0x01 },
};
static const struct bt_mesh_test_cfg rx_cfg = {
	.addr = 0x0002,
	.dev_key = { 0x02 },
};

static void test_tx_init(void)
{
	bt_mesh_test_cfg_set(&tx_cfg, WAIT_TIME);
}

static void test_rx_init(void)
{
	bt_mesh_test_cfg_set(&rx_cfg, WAIT_TIME);
}

/* Setup the tx device by enabling Bluetooth, but no scanner is needed. */
static void test_tx_device_setup(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)", err);
		return;
	}

	LOG_INF("Bluetooth initialized");
}

/** Bypassing setting up transmission, and will try to send the raw data that is
 *  provided to the function.
 */
static void test_tx_send_ad_type_msg(uint8_t type, const uint8_t *data, uint8_t len)
{
	struct bt_le_adv_param param = {};
	uint16_t duration, adv_int;
	struct bt_data ad;
	int err;

	adv_int = ADV_INT_FAST_MS;
	duration = ADV_DURATION;

	ad.type = type;
	ad.data_len = len;
	ad.data = data;

	param.id = BT_ID_DEFAULT;
	param.interval_min = BT_MESH_ADV_SCAN_UNIT(adv_int);
	param.interval_max = param.interval_min;

	LOG_DBG("ad.type %u len %u", ad.type, ad.data_len);
	LOG_HEXDUMP_DBG(ad.data, ad.data_len, "ad.data");

	uint64_t st = k_uptime_get();

	err = bt_le_adv_start(&param, &ad, 1, NULL, 0);
	if (err) {
		FAIL("Advertising failed: err %d", err);
		return;
	}
	LOG_DBG("Advertising started. Sleeping %u ms", duration);

	k_sleep(K_MSEC(duration));
	err = bt_le_adv_stop();
	if (err) {
		FAIL("Stopping advertising failed: err %d", err);
		return;
	}
	LOG_DBG("Advertising stopped (%u ms)", (uint32_t) k_uptime_delta(&st));
}

/** Test sending message with not supported ad type for mesh.
 *
 * First message send message with an ad data type not supported by mesh, and
 * verify that the receiver is disregarding message. Then send the same message
 * with correct ad type to verify that the message is received.
 */
static void test_tx_invalid_ad_type(void)
{
	test_tx_device_setup();

	LOG_DBG("TX Invalid AD Type");

	k_sleep(K_SECONDS(1));

	/* Send message with invalid ad type. */
	test_tx_send_ad_type_msg(BT_DATA_BIG_INFO, valid_message,
				 ARRAY_SIZE(valid_message));

	/* Wait for no message receive window to end. */
	k_sleep(K_SECONDS(10));

	/* Send message with valid ad type to verify message. */
	test_tx_send_ad_type_msg(BT_DATA_MESH_MESSAGE, valid_message,
				 ARRAY_SIZE(valid_message));

	PASS();
}

/** Test sending message with invalid/valid ad type and wrong packet length.
 *
 * Send messages with wrong packet length, and verify that the receiver is
 * disregarding message. Then send the same message with correct packet length
 * to verify that the message is received.
 */
static void test_tx_wrong_packet_length(void)
{
	test_tx_device_setup();

	LOG_DBG("TX wrong packet length");

	k_sleep(K_SECONDS(1));

	/* Send message with to long data length. */
	test_tx_send_ad_type_msg(BT_DATA_MESH_MESSAGE, valid_message,
				 ARRAY_SIZE(valid_message)+1);
	/* Send message with to short data length. */
	test_tx_send_ad_type_msg(BT_DATA_MESH_MESSAGE, valid_message,
				 ARRAY_SIZE(valid_message)-1);
	/* Send message with invalid ad type and wrong data length. */
	test_tx_send_ad_type_msg(BT_DATA_BIG_INFO, valid_message,
				 ARRAY_SIZE(valid_message)+1);

	/* Wait for no message receive window to end. */
	k_sleep(K_SECONDS(10));

	/* Send message with valid ad type to verify message. */
	test_tx_send_ad_type_msg(BT_DATA_MESH_MESSAGE, valid_message,
				 ARRAY_SIZE(valid_message));

	PASS();
}

/** Test receiving message with invalid ad type or packet length for mesh. */
static void test_rx_invalid_packet(void)
{
	struct bt_mesh_test_msg msg;
	int err;

	bt_mesh_test_setup();

	LOG_DBG("RX Invalid packet");

	/* Wait to check that no valid messages is received. */
	err = bt_mesh_test_recv_msg(&msg, K_SECONDS(10));
	if (!err) {
		FAIL("Unexpected rx from 0x%04x", msg.ctx.addr);
	}

	/* Verify that test data is received correct. */
	err = bt_mesh_test_recv(10, cfg->addr, K_SECONDS(10));
	ASSERT_OK(err, "Failed receiving with valid ad_type");

	PASS();
}

#define TEST_CASE(role, name, description)                     \
	{                                                      \
		.test_id = "scanner_" #role "_" #name,         \
		.test_descr = description,                     \
		.test_post_init_f = test_##role##_init,        \
		.test_tick_f = bt_mesh_test_timeout,           \
		.test_main_f = test_##role##_##name,           \
	}

static const struct bst_test_instance test_scanner[] = {
	TEST_CASE(tx, invalid_ad_type,     "Scanner: Invalid AD Type"),
	TEST_CASE(tx, wrong_packet_length, "Scanner: Wrong data length"),

	TEST_CASE(rx, invalid_packet,      "Scanner: Invalid packet"),
	BSTEST_END_MARKER
};

struct bst_test_list *test_scanner_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_scanner);
	return tests;
}
