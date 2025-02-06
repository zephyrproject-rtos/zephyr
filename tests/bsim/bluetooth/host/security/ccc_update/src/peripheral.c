/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <zephyr/logging/log.h>

#include <zephyr/settings/settings.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"
#include "common.h"
#include "settings.h"

#include "argparse.h"
#include "bs_pc_backchannel.h"

#define GOOD_CLIENT_CHAN 0
#define BAD_CLIENT_CHAN 1

DEFINE_FLAG_STATIC(connected_flag);
DEFINE_FLAG_STATIC(disconnected_flag);
DEFINE_FLAG_STATIC(security_updated_flag);
DEFINE_FLAG_STATIC(ccc_cfg_changed_flag);

static const struct bt_uuid_128 dummy_service = BT_UUID_INIT_128(DUMMY_SERVICE_TYPE);

static const struct bt_uuid_128 notify_characteristic_uuid =
					BT_UUID_INIT_128(DUMMY_SERVICE_NOTIFY_TYPE);

static struct bt_conn *default_conn;

static struct bt_conn_cb peripheral_cb;

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);

	LOG_INF("CCC Update: notification %s", notif_enabled ? "enabled" : "disabled");

	SET_FLAG(ccc_cfg_changed_flag);
}

BT_GATT_SERVICE_DEFINE(dummy_svc, BT_GATT_PRIMARY_SERVICE(&dummy_service),
		       BT_GATT_CHARACTERISTIC(&notify_characteristic_uuid.uuid, BT_GATT_CHRC_NOTIFY,
					      BT_GATT_PERM_NONE, NULL, NULL, NULL),
		       BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

static void create_adv(struct bt_le_ext_adv **adv)
{
	int err;
	struct bt_le_adv_param params;

	memset(&params, 0, sizeof(struct bt_le_adv_param));

	params.options |= BT_LE_ADV_OPT_CONN;

	params.id = BT_ID_DEFAULT;
	params.sid = 0;
	params.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
	params.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;

	err = bt_le_ext_adv_create(&params, NULL, adv);
	if (err) {
		TEST_FAIL("Failed to create advertiser (%d)", err);
	}
}

static void start_adv(struct bt_le_ext_adv *adv)
{
	int err;
	int32_t timeout = 0;
	int8_t num_events = 0;

	struct bt_le_ext_adv_start_param start_params;

	start_params.timeout = timeout;
	start_params.num_events = num_events;

	err = bt_le_ext_adv_start(adv, &start_params);
	if (err) {
		TEST_FAIL("Failed to start advertiser (err %d)", err);
	}

	LOG_DBG("Advertiser started");
}

static void stop_adv(struct bt_le_ext_adv *adv)
{
	int err;

	err = bt_le_ext_adv_stop(adv);
	if (err) {
		TEST_FAIL("Failed to stop advertiser (err %d)", err);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

	if (err) {
		TEST_FAIL("Failed to connect to %s (err %d)", addr_str, err);
	}

	LOG_DBG("Connected: %s", addr_str);

	default_conn = bt_conn_ref(conn);

	SET_FLAG(connected_flag);
	UNSET_FLAG(disconnected_flag);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

	LOG_DBG("Disconnected: %s (reason 0x%02x)", addr_str, reason);

	bt_conn_unref(conn);
	default_conn = NULL;

	SET_FLAG(disconnected_flag);
	UNSET_FLAG(connected_flag);
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

	if (!err) {
		LOG_DBG("Security changed: %s level %u", addr_str, level);
		SET_FLAG(security_updated_flag);
	} else {
		LOG_DBG("Security failed: %s level %u err %d", addr_str, level, err);
	}
}

static bool is_peer_subscribed(struct bt_conn *conn)
{
	struct bt_gatt_attr *attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_DUMMY_SERVICE_NOTIFY);
	bool is_subscribed = bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY);

	return is_subscribed;
}

/* Test steps */

static void connect_pair_check_subscribtion(struct bt_le_ext_adv *adv)
{
	start_adv(adv);

	WAIT_FOR_FLAG(connected_flag);

	WAIT_FOR_FLAG(security_updated_flag);
	UNSET_FLAG(security_updated_flag);

	/* wait for confirmation of subscribtion from good client */
	backchannel_sync_wait(GOOD_CLIENT_CHAN, GOOD_CLIENT_ID);

	/* check that subscribtion request did not fail */
	if (!is_peer_subscribed(default_conn)) {
		TEST_FAIL("Good client did not subscribed");
	}

	stop_adv(adv);

	/* confirm to good client that the subscribtion has been well registered */
	backchannel_sync_send(GOOD_CLIENT_CHAN, GOOD_CLIENT_ID);
}

static void connect_wait_unsubscribtion(struct bt_le_ext_adv *adv)
{
	UNSET_FLAG(ccc_cfg_changed_flag);

	start_adv(adv);

	WAIT_FOR_FLAG(connected_flag);

	stop_adv(adv);

	/* check that subscribtion is restored for bad client */
	if (!is_peer_subscribed(default_conn)) {
		TEST_FAIL("Subscribtion has not been restored for bad client");
	}

	/* confirm to bad client that the subscribtion had not been restored */
	backchannel_sync_send(BAD_CLIENT_CHAN, BAD_CLIENT_ID);
	/* wait for confirmation that bad client requested unsubscribtion */
	backchannel_sync_wait(BAD_CLIENT_CHAN, BAD_CLIENT_ID);

	/* check that unsubscribtion request didn't fail */
	if (!IS_FLAG_SET(ccc_cfg_changed_flag)) {
		TEST_FAIL("Bad client didn't manage to update CCC config");
	}

	/* confirm to bad client that unsubscribtion request has been well registered */
	backchannel_sync_send(BAD_CLIENT_CHAN, BAD_CLIENT_ID);
}

static void connect_restore_sec_check_subscribtion(struct bt_le_ext_adv *adv)
{
	start_adv(adv);

	WAIT_FOR_FLAG(connected_flag);

	WAIT_FOR_FLAG(security_updated_flag);
	UNSET_FLAG(security_updated_flag);

	/* wait for good client end of security update */
	backchannel_sync_wait(GOOD_CLIENT_CHAN, GOOD_CLIENT_ID);

	/* check that subscribtion hasn't been restored */
	if (is_peer_subscribed(default_conn)) {
		TEST_FAIL("Good client is subscribed");
	}

	/* confirm to good client that the subscribtion has been well restored */
	backchannel_sync_send(GOOD_CLIENT_CHAN, GOOD_CLIENT_ID);
	/* wait for confimation of unsubscribtion from good client */
	backchannel_sync_wait(GOOD_CLIENT_CHAN, GOOD_CLIENT_ID);

	/* check that unsubscribtion request from good client has been registered */
	if (is_peer_subscribed(default_conn)) {
		TEST_FAIL("Good client did not unsubscribe");
	}
}

/* Util functions */

void peripheral_backchannel_init(void)
{
	uint device_number = get_device_nbr();
	uint channel_numbers[2] = {
		0,
		0,
	};
	uint device_numbers[2] = {
		GOOD_CLIENT_ID,
		BAD_CLIENT_ID,
	};
	uint num_ch = 2;
	uint *ch;

	LOG_DBG("Opening back channels for device %d", device_number);
	ch = bs_open_back_channel(device_number, device_numbers, channel_numbers, num_ch);
	if (!ch) {
		TEST_FAIL("Unable to open backchannel");
	}
}

static void check_ccc_handle(void)
{
	struct bt_gatt_attr *service_notify_attr =
		bt_gatt_find_by_uuid(NULL, 0, &notify_characteristic_uuid.uuid);

	struct bt_gatt_attr *ccc_attr =
		bt_gatt_find_by_uuid(service_notify_attr, 0, BT_UUID_GATT_CCC);
	uint16_t actual_ccc_handle = bt_gatt_attr_get_handle(ccc_attr);

	__ASSERT(actual_ccc_handle == CCC_HANDLE,
		 "Please update the CCC_HANDLE define (actual_ccc_handle=%d)", actual_ccc_handle);
}

/* Main function */

void run_peripheral(void)
{
	/*
	 * test goal: demonstrate the expected behavior of the GATT server when
	 * a non-bonded peer try to unsubscribe from a previously subscription
	 * done in a bonded context
	 *
	 * test pass if the bad client manage to unsubscribe
	 */

	int err;
	struct bt_le_ext_adv *adv = NULL;

	peripheral_cb.connected = connected;
	peripheral_cb.disconnected = disconnected;
	peripheral_cb.security_changed = security_changed;

	peripheral_backchannel_init();

	err = bt_enable(NULL);
	if (err) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);
	}

	LOG_DBG("Bluetooth initialized");

	check_ccc_handle();

	bt_conn_cb_register(&peripheral_cb);

	err = settings_load();
	if (err) {
		TEST_FAIL("Settings load failed (err %d)", err);
	}

	err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
	if (err) {
		TEST_FAIL("Unpairing failed (err %d)", err);
	}

	create_adv(&adv);

	connect_pair_check_subscribtion(adv);
	WAIT_FOR_FLAG(disconnected_flag);

	connect_wait_unsubscribtion(adv);
	WAIT_FOR_FLAG(disconnected_flag);

	connect_restore_sec_check_subscribtion(adv);
	WAIT_FOR_FLAG(disconnected_flag);

	TEST_PASS("Peripheral test passed");
}
