/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"
#include "common.h"

DEFINE_FLAG_STATIC(flag_connected);
DEFINE_FLAG_STATIC(flag_notify_enabled);
DEFINE_FLAG_STATIC(flag_indicate_enabled);

static struct bt_conn *g_conn;

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0) {
		TEST_FAIL("Failed to connect to %s (%u)", bt_conn_dst_str(conn), err);
		return;
	}

	g_conn = bt_conn_ref(conn);
	SET_FLAG(flag_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn != g_conn) {
		return;
	}

	bt_conn_drop(&g_conn);
	UNSET_FLAG(flag_connected);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void notify_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	if (value == BT_GATT_CCC_NOTIFY) {
		SET_FLAG(flag_notify_enabled);
	}
}

static void indicate_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	if (value == BT_GATT_CCC_INDICATE) {
		SET_FLAG(flag_indicate_enabled);
	}
}

static void other_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
}

BT_GATT_SERVICE_DEFINE(test_svc,
	BT_GATT_PRIMARY_SERVICE(TEST_SERVICE_UUID),
	BT_GATT_CHARACTERISTIC(TEST_NOTIFY_CHRC_UUID, BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(notify_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(TEST_INDICATE_CHRC_UUID, BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(indicate_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(TEST_BOTH_CHRC_UUID,
			       BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(other_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/* A CCC with no characteristic declaration in front of it, to exercise the
 * fallback that lets the write through when the properties cannot be looked
 * up.
 */
BT_GATT_SERVICE_DEFINE(bare_svc,
	BT_GATT_PRIMARY_SERVICE(TEST_BARE_SERVICE_UUID),
	BT_GATT_CCC(other_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static void indicate_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
	if (err != 0) {
		TEST_FAIL("Indication failed (err %u)", err);
	}
}

static void test_main(void)
{
	int err;
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR))
	};
	static struct bt_gatt_indicate_params ind_params;
	const uint8_t data = 0x2b;

	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		TEST_FAIL("Advertising failed to start (err %d)", err);
		return;
	}

	WAIT_FOR_FLAG(flag_connected);

	WAIT_FOR_FLAG(flag_notify_enabled);
	/* attrs[2] is the notify characteristic value */
	err = bt_gatt_notify(NULL, &test_svc.attrs[2], &data, sizeof(data));
	if (err != 0) {
		TEST_FAIL("Notify failed (err %d)", err);
	}

	WAIT_FOR_FLAG(flag_indicate_enabled);
	/* attrs[5] is the indicate characteristic value */
	ind_params.attr = &test_svc.attrs[5];
	ind_params.func = indicate_cb;
	ind_params.data = &data;
	ind_params.len = sizeof(data);
	err = bt_gatt_indicate(NULL, &ind_params);
	if (err != 0) {
		TEST_FAIL("Indicate failed (err %d)", err);
	}

	WAIT_FOR_FLAG_UNSET(flag_connected);

	TEST_PASS("GATT server passed");
}

static const struct bst_test_instance test_gatt_server[] = {
	{
		.test_id = "gatt_server",
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_gatt_server_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_gatt_server);
}
