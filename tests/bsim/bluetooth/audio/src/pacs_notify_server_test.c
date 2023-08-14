/*
 * Copyright (c) 2023 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/pacs.h>

#include "common.h"

extern enum bst_result_t bst_result;

static struct bt_audio_codec_cap lc3_codec_1 =
	BT_AUDIO_CODEC_CAP_LC3(BT_AUDIO_CODEC_LC3_FREQ_16KHZ | BT_AUDIO_CODEC_LC3_FREQ_24KHZ,
			   BT_AUDIO_CODEC_LC3_DURATION_10,
			   BT_AUDIO_CODEC_LC3_CHAN_COUNT_SUPPORT(1), 40u, 60u, 1u,
			   BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_audio_codec_cap lc3_codec_2 =
	BT_AUDIO_CODEC_CAP_LC3(BT_AUDIO_CODEC_LC3_FREQ_16KHZ,
			   BT_AUDIO_CODEC_LC3_DURATION_10,
			   BT_AUDIO_CODEC_LC3_CHAN_COUNT_SUPPORT(1), 40u, 60u, 1u,
			   BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_pacs_cap                    caps_1 = {
	.codec_cap = &lc3_codec_1,
};
static struct bt_pacs_cap                    caps_2 = {
	.codec_cap = &lc3_codec_2,
};

static bool is_peer_subscribed(struct bt_conn *conn)
{
	struct bt_gatt_attr *attr;
	uint8_t nbr_subscribed = 0;

	attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SNK);
	if (!attr) {
		printk("No BT_UUID_PACS_SNK attribute found\n");
	}
	if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		nbr_subscribed++;
	}

	attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SNK_LOC);
	if (!attr) {
		printk("No BT_UUID_PACS_SNK_LOC attribute found\n");
	}
	if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		nbr_subscribed++;
	}

	attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SRC);
	if (!attr) {
		printk("No BT_UUID_PACS_SRC attribute found\n");
	}
	if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		nbr_subscribed++;
	}

	attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SRC_LOC);
	if (!attr) {
		printk("No BT_UUID_PACS_SRC_LOC attribute found\n");
	}
	if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		nbr_subscribed++;
	}

	attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_AVAILABLE_CONTEXT);
	if (!attr) {
		printk("No BT_UUID_PACS_AVAILABLE_CONTEXT attribute found\n");
	}
	if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		nbr_subscribed++;
	}

	attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SUPPORTED_CONTEXT);
	if (!attr) {
		printk("No BT_UUID_PACS_SUPPORTED_CONTEXT attribute found\n");
	}
	if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		nbr_subscribed++;
	}

	if (nbr_subscribed != 6) {
		return false;
	}

	return true;
}

static void trigger_notifications(void)
{
	static enum bt_audio_context available = BT_AUDIO_CONTEXT_TYPE_ANY;
	static enum bt_audio_context supported = BT_AUDIO_CONTEXT_TYPE_ANY;
	static int i;
	int err;
	struct bt_pacs_cap *caps;
	enum bt_audio_location snk_loc;
	enum bt_audio_location src_loc;

	printk("Triggering Notifications\n");

	if (i) {
		caps = &caps_1;
		snk_loc = BT_AUDIO_LOCATION_FRONT_LEFT;
		src_loc = BT_AUDIO_LOCATION_FRONT_RIGHT;
		i = 0;
	} else {
		caps = &caps_2;
		snk_loc = BT_AUDIO_LOCATION_FRONT_RIGHT;
		src_loc = BT_AUDIO_LOCATION_FRONT_LEFT;
		i++;
	}

	printk("Changing Sink PACs\n");
	bt_pacs_cap_register(BT_AUDIO_DIR_SINK, caps);
	bt_pacs_cap_register(BT_AUDIO_DIR_SOURCE, caps);

	printk("Changing Sink Location\n");
	err = bt_pacs_set_location(BT_AUDIO_DIR_SINK, snk_loc);
	if (err != 0) {
		printk("Failed to set device sink location\n");
	}

	printk("Changing Source Location\n");
	err = bt_pacs_set_location(BT_AUDIO_DIR_SOURCE, src_loc);
	if (err != 0) {
		printk("Failed to set device source location\n");
	}

	printk("Changing Supported Contexts Location\n");
	supported = supported ^ BT_AUDIO_CONTEXT_TYPE_MEDIA;
	bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SINK, supported);

	printk("Changing Available Contexts\n");
	available = available ^ BT_AUDIO_CONTEXT_TYPE_MEDIA;
	bt_pacs_set_available_contexts(BT_AUDIO_DIR_SINK, available);
}

static void test_main(void)
{
	int err;
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	};

	printk("Enabling Bluetooth\n");
	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SINK, BT_AUDIO_CONTEXT_TYPE_ANY);
	bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SOURCE, BT_AUDIO_CONTEXT_TYPE_ANY);
	bt_pacs_set_available_contexts(BT_AUDIO_DIR_SINK, BT_AUDIO_CONTEXT_TYPE_ANY);
	bt_pacs_set_available_contexts(BT_AUDIO_DIR_SOURCE, BT_AUDIO_CONTEXT_TYPE_ANY);

	printk("Registereding PACS\n");
	bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &caps_1);
	bt_pacs_cap_register(BT_AUDIO_DIR_SOURCE, &caps_1);

	err = bt_pacs_set_location(BT_AUDIO_DIR_SINK, BT_AUDIO_LOCATION_FRONT_LEFT);
	if (err != 0) {
		printk("Failed to set device sink location\n");
		return;
	}

	err = bt_pacs_set_location(BT_AUDIO_DIR_SOURCE, BT_AUDIO_LOCATION_FRONT_RIGHT);
	if (err != 0) {
		printk("Failed to set device source location\n");
		return;
	}

	printk("Start Advertising\n");
	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Waiting to be connected\n");
	WAIT_FOR_FLAG(flag_connected);
	printk("Connected\n");
	printk("Waiting to be subscribed\n");

	while (!is_peer_subscribed(default_conn)) {
		(void)k_sleep(K_MSEC(10));
	}
	printk("Subscribed\n");

	trigger_notifications();

	/* Now wait for client to disconnect, then stop adv so it does not reconnect */
	printk("Wait for client disconnect\n");
	WAIT_FOR_UNSET_FLAG(flag_connected);
	printk("Client disconnected\n");

	err = bt_le_adv_stop();
	if (err != 0) {
		FAIL("Advertising failed to stop (err %d)\n", err);
		return;
	}

	/* Trigger changes while device is disconnected */
	trigger_notifications();

	printk("Start Advertising\n");
	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_connected);
	WAIT_FOR_UNSET_FLAG(flag_connected);

	PASS("PACS Notify Server passed\n");
}

static const struct bst_test_instance test_pacs_notify_server[] = {
	{
		.test_id = "pacs_notify_server",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_pacs_notify_server_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_pacs_notify_server);
}
