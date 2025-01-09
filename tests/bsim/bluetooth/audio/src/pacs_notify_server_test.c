/*
 * Copyright (c) 2023 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/lc3.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

#include "bstests.h"
#include "common.h"

LOG_MODULE_REGISTER(pacs_notify_server_test, LOG_LEVEL_DBG);

extern enum bst_result_t bst_result;

static struct bt_audio_codec_cap lc3_codec_1 =
	BT_AUDIO_CODEC_CAP_LC3(BT_AUDIO_CODEC_CAP_FREQ_16KHZ | BT_AUDIO_CODEC_CAP_FREQ_24KHZ,
			   BT_AUDIO_CODEC_CAP_DURATION_10,
			   BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), 40u, 60u, 1u,
			   BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_audio_codec_cap lc3_codec_2 =
	BT_AUDIO_CODEC_CAP_LC3(BT_AUDIO_CODEC_CAP_FREQ_16KHZ,
			   BT_AUDIO_CODEC_CAP_DURATION_10,
			   BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), 40u, 60u, 1u,
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
		LOG_DBG("No BT_UUID_PACS_SNK attribute found");
	}
	if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		nbr_subscribed++;
	}

	attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SNK_LOC);
	if (!attr) {
		LOG_DBG("No BT_UUID_PACS_SNK_LOC attribute found");
	}
	if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		nbr_subscribed++;
	}

	attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SRC);
	if (!attr) {
		LOG_DBG("No BT_UUID_PACS_SRC attribute found");
	}
	if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		nbr_subscribed++;
	}

	attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SRC_LOC);
	if (!attr) {
		LOG_DBG("No BT_UUID_PACS_SRC_LOC attribute found");
	}
	if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		nbr_subscribed++;
	}

	attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_AVAILABLE_CONTEXT);
	if (!attr) {
		LOG_DBG("No BT_UUID_PACS_AVAILABLE_CONTEXT attribute found");
	}
	if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		nbr_subscribed++;
	}

	attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_PACS_SUPPORTED_CONTEXT);
	if (!attr) {
		LOG_DBG("No BT_UUID_PACS_SUPPORTED_CONTEXT attribute found");
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

	LOG_DBG("Triggering Notifications");

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

	LOG_DBG("Changing Sink PACs");
	bt_pacs_cap_register(BT_AUDIO_DIR_SINK, caps);
	bt_pacs_cap_register(BT_AUDIO_DIR_SOURCE, caps);

	LOG_DBG("Changing Sink Location");
	err = bt_pacs_set_location(BT_AUDIO_DIR_SINK, snk_loc);
	if (err != 0) {
		LOG_DBG("Failed to set device sink location");
	}

	LOG_DBG("Changing Source Location");
	err = bt_pacs_set_location(BT_AUDIO_DIR_SOURCE, src_loc);
	if (err != 0) {
		LOG_DBG("Failed to set device source location");
	}

	LOG_DBG("Changing Supported Contexts Location");
	supported = supported ^ BT_AUDIO_CONTEXT_TYPE_MEDIA;
	bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SINK, supported);

	LOG_DBG("Changing Available Contexts");
	available = available ^ BT_AUDIO_CONTEXT_TYPE_MEDIA;
	bt_pacs_set_available_contexts(BT_AUDIO_DIR_SINK, available);
}

static void test_main(void)
{
	int err;
	enum bt_audio_context available, available_for_conn;
	struct bt_le_ext_adv *ext_adv;

	LOG_DBG("Enabling Bluetooth");
	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)", err);
		return;
	}

	bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SINK, BT_AUDIO_CONTEXT_TYPE_ANY);
	bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SOURCE, BT_AUDIO_CONTEXT_TYPE_ANY);
	bt_pacs_set_available_contexts(BT_AUDIO_DIR_SINK, BT_AUDIO_CONTEXT_TYPE_ANY);
	bt_pacs_set_available_contexts(BT_AUDIO_DIR_SOURCE, BT_AUDIO_CONTEXT_TYPE_ANY);

	LOG_DBG("Registereding PACS");
	bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &caps_1);
	bt_pacs_cap_register(BT_AUDIO_DIR_SOURCE, &caps_1);

	err = bt_pacs_set_location(BT_AUDIO_DIR_SINK, BT_AUDIO_LOCATION_FRONT_LEFT);
	if (err != 0) {
		LOG_DBG("Failed to set device sink location");
		return;
	}

	err = bt_pacs_set_location(BT_AUDIO_DIR_SOURCE, BT_AUDIO_LOCATION_FRONT_RIGHT);
	if (err != 0) {
		LOG_DBG("Failed to set device source location");
		return;
	}

	LOG_DBG("Start Advertising");
	setup_connectable_adv(&ext_adv);

	LOG_DBG("Waiting to be connected");
	WAIT_FOR_FLAG(flag_connected);
	LOG_DBG("Connected");
	LOG_DBG("Waiting to be subscribed");

	while (!is_peer_subscribed(default_conn)) {
		(void)k_sleep(K_MSEC(10));
	}
	LOG_DBG("Subscribed");

	LOG_INF("Trigger changes while device is connected");
	trigger_notifications();

	/* Now wait for client to disconnect */
	LOG_DBG("Wait for client disconnect");
	WAIT_FOR_UNSET_FLAG(flag_connected);
	LOG_DBG("Client disconnected");

	LOG_INF("Trigger changes while device is disconnected");
	trigger_notifications();

	LOG_DBG("Start Advertising");
	err = bt_le_ext_adv_start(ext_adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		FAIL("Failed to start advertising set (err %d)\n", err);

		bt_le_ext_adv_delete(ext_adv);

		return;
	}

	WAIT_FOR_FLAG(flag_connected);
	WAIT_FOR_UNSET_FLAG(flag_connected);
	LOG_DBG("Client disconnected");

	LOG_DBG("Start Advertising");
	err = bt_le_ext_adv_start(ext_adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		FAIL("Failed to start advertising set (err %d)\n", err);

		bt_le_ext_adv_delete(ext_adv);

		return;
	}

	WAIT_FOR_FLAG(flag_connected);
	LOG_DBG("Connected");

	available = bt_pacs_get_available_contexts(BT_AUDIO_DIR_SINK);
	__ASSERT_NO_MSG(bt_pacs_get_available_contexts_for_conn(default_conn, BT_AUDIO_DIR_SINK) ==
			available);

	available_for_conn = BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED;

	LOG_INF("Override available contexts");
	err = bt_pacs_conn_set_available_contexts_for_conn(default_conn, BT_AUDIO_DIR_SINK,
							   &available_for_conn);
	__ASSERT_NO_MSG(err == 0);

	__ASSERT_NO_MSG(bt_pacs_get_available_contexts(BT_AUDIO_DIR_SINK) == available);
	__ASSERT_NO_MSG(bt_pacs_get_available_contexts_for_conn(default_conn, BT_AUDIO_DIR_SINK) ==
			available_for_conn);

	WAIT_FOR_UNSET_FLAG(flag_connected);
	LOG_DBG("Client disconnected");

	LOG_DBG("Start Advertising");
	err = bt_le_ext_adv_start(ext_adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		FAIL("Failed to start advertising set (err %d)\n", err);

		bt_le_ext_adv_delete(ext_adv);

		return;
	}

	WAIT_FOR_FLAG(flag_connected);
	LOG_DBG("Connected");

	__ASSERT_NO_MSG(bt_pacs_get_available_contexts(BT_AUDIO_DIR_SINK) == available);
	__ASSERT_NO_MSG(bt_pacs_get_available_contexts_for_conn(default_conn, BT_AUDIO_DIR_SINK) ==
			available);

	WAIT_FOR_UNSET_FLAG(flag_connected);

	PASS("PACS Notify Server passed\n");
}

static const struct bst_test_instance test_pacs_notify_server[] = {
	{
		.test_id = "pacs_notify_server",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_pacs_notify_server_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_pacs_notify_server);
}
