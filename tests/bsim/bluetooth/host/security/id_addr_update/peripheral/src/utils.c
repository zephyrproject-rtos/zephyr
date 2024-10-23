/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include "utils.h"

BUILD_ASSERT(CONFIG_BT_MAX_PAIRED >= 2, "CONFIG_BT_MAX_PAIRED is too small.");
BUILD_ASSERT(CONFIG_BT_ID_MAX >= 3, "CONFIG_BT_ID_MAX is too small.");
BUILD_ASSERT(CONFIG_BT_MAX_CONN == 2, "CONFIG_BT_MAX_CONN should be equal to two.");

void test_tick(bs_time_t HW_device_time)
{
	bs_trace_debug_time(0, "Simulation ends now.\n");
	if (bst_result != Passed) {
		bst_result = Failed;
		bs_trace_error("Test did not pass before simulation ended.\n");
	}
}

void test_init(void)
{
	bst_ticker_set_next_tick_absolute(TEST_TIMEOUT_SIMULATED);
	bst_result = In_progress;
}

DEFINE_FLAG(flag_has_new_conn);
struct bt_conn *new_conn;

DEFINE_FLAG(flag_has_disconnected);

void clear_conn(struct bt_conn *conn)
{
	if (new_conn == conn) {
		new_conn = NULL;
	}

	ASSERT(conn, "Test error: No new_conn!\n");
	bt_conn_unref(conn);
}

void wait_connected(struct bt_conn **conn)
{
	WAIT_FOR_FLAG(flag_has_new_conn);
	UNSET_FLAG(flag_has_new_conn);

	ASSERT(new_conn, "connection unpopulated.");
	*conn = new_conn;
	new_conn = NULL;
}

void wait_disconnected(void)
{
	WAIT_FOR_FLAG(flag_has_disconnected);
	UNSET_FLAG(flag_has_disconnected);
}

static void print_conn_state_transition(const char *prefix, struct bt_conn *conn)
{
	int err;
	struct bt_conn_info info;
	char addr_str[BT_ADDR_LE_STR_LEN];

	err = bt_conn_get_info(conn, &info);
	ASSERT(!err, "Unexpected conn info result.");

	bt_addr_le_to_str(info.le.dst, addr_str, sizeof(addr_str));
	printk("%s: %s\n", prefix, addr_str);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	print_conn_state_transition("Disonnected", conn);
	SET_FLAG(flag_has_disconnected);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	ASSERT((!new_conn || (conn == new_conn)), "Unexpected new connection.");

	if (!new_conn) {
		new_conn = bt_conn_ref(conn);
	}

	if (err != 0) {
		clear_conn(conn);
		return;
	}

	print_conn_state_transition("Connected", conn);
	SET_FLAG(flag_has_new_conn);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

DEFINE_FLAG(flag_pairing_completed);

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	print_conn_state_transition("Paired", conn);
	SET_FLAG(flag_pairing_completed);
}

static struct bt_conn_auth_info_cb bt_conn_auth_info_cb = {
	.pairing_complete = pairing_complete,
};

void set_security(struct bt_conn *conn, bt_security_t sec)
{
	int err;

	err = bt_conn_set_security(conn, sec);
	ASSERT(!err, "Err bt_conn_set_security %d", err);
}

void wait_pairing_completed(void)
{
	WAIT_FOR_FLAG(flag_pairing_completed);
	UNSET_FLAG(flag_pairing_completed);
}

void bs_bt_utils_setup(void)
{
	int err;

	err = bt_enable(NULL);
	ASSERT(!err, "bt_enable failed.\n");
	err = bt_conn_auth_info_cb_register(&bt_conn_auth_info_cb);
	ASSERT(!err, "bt_conn_auth_info_cb_register failed.\n");

	err = settings_load();
	ASSERT(!err, "settings_load failed.\n");
}

void disconnect(struct bt_conn *conn)
{
	int err;

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	ASSERT(!err, "Err bt_conn_disconnect %d", err);
}

void advertise_connectable(int id)
{
	int err;
	struct bt_le_adv_param param = {};

	param.id = id;
	param.interval_min = 0x0020;
	param.interval_max = 0x4000;
	param.options |= BT_LE_ADV_OPT_CONN;

	err = bt_le_adv_start(&param, NULL, 0, NULL, 0);
	ASSERT(!err, "Advertising failed to start (err %d)\n", err);
}

DEFINE_FLAG(flag_bas_ccc_subscribed);
static uint8_t bas_level = 50;

static void bas_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				uint16_t value)
{
	ARG_UNUSED(attr);

	if (value == BT_GATT_CCC_NOTIFY) {
		printk("BAS CCCD: notification enabled\n");
		SET_FLAG(flag_bas_ccc_subscribed);
	}
}

void wait_bas_ccc_subscription(void)
{
	WAIT_FOR_FLAG(flag_bas_ccc_subscribed);
	UNSET_FLAG(flag_bas_ccc_subscribed);
}

static ssize_t bas_read(struct bt_conn *conn,
			const struct bt_gatt_attr *attr, void *buf,
			uint16_t len, uint16_t offset)
{
	uint8_t lvl8 = bas_level;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &lvl8,
				 sizeof(lvl8));
}

BT_GATT_SERVICE_DEFINE(bas,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, bas_read, NULL, &bas_level),
	BT_GATT_CCC(bas_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

void bas_notify(struct bt_conn *conn)
{
	int err;

	err = bt_gatt_notify(conn, &bas.attrs[2], &bas_level, sizeof(bas_level));
	ASSERT(!err, "bt_gatt_notify failed (err %d)\n", err);
}

DEFINE_FLAG(flag_bas_has_notification);
