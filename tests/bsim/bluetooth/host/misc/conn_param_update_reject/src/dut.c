/*
 * DUT (peripheral) side of the conn_param_update_reject bsim test.
 *
 * Copyright (c) 2026 Sharon Lin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>

#include <testlib/conn.h>
#include "babblekit/testcase.h"
#include "babblekit/flags.h"
#include "babblekit/sync.h"

DEFINE_FLAG_STATIC(flag_is_connected);
DEFINE_FLAG_STATIC(flag_param_rejected);

static struct bt_conn *g_conn;

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0) {
		TEST_FAIL("Failed to connect to %s (%u)", bt_conn_dst_str(conn), err);
		return;
	}

	printk("Connected to %s\n", bt_conn_dst_str(conn));

	__ASSERT_NO_MSG(g_conn == NULL);
	g_conn = bt_conn_ref(conn);
	SET_FLAG(flag_is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn != g_conn) {
		return;
	}

	printk("Disconnected: %s (reason 0x%02x)\n", bt_conn_dst_str(conn), reason);

	bt_conn_unref(g_conn);
	g_conn = NULL;
	UNSET_FLAG(flag_is_connected);
}

static void le_param_update_rejected(struct bt_conn *conn, uint8_t hci_err)
{
	printk("Connection parameter update rejected, hci_err 0x%02x\n", hci_err);

	if (hci_err == 0) {
		TEST_FAIL("Rejection reported with a success status");
		return;
	}

	SET_FLAG(flag_param_rejected);
}

static struct bt_conn_cb dut_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_update_rejected = le_param_update_rejected,
};

void test_peripheral_main(void)
{
	int err;
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR))};
	struct bt_le_conn_param *param = BT_LE_CONN_PARAM(0x0028, 0x0038, 0, 400);

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);
		return;
	}

	err = bk_sync_init();
	if (err != 0) {
		TEST_FAIL("Failed to initialize backchannel (err %d)", err);
		return;
	}

	err = bt_conn_cb_register(&dut_callbacks);
	__ASSERT_NO_MSG(err == 0);

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		TEST_FAIL("Advertising failed to start (err %d)", err);
		return;
	}

	WAIT_FOR_FLAG(flag_is_connected);

	/* Give the feature exchange time to complete before requesting an update */
	k_sleep(K_SECONDS(1));

	err = bt_conn_le_param_update(g_conn, param);
	if (err != 0) {
		TEST_FAIL("Parameter update request failed (err %d)", err);
		return;
	}

	WAIT_FOR_FLAG(flag_param_rejected);

	bk_sync_send();
	TEST_PASS("Peripheral received connection parameter update rejection");
}

void test_peripheral_auto_main(void)
{
	int err;
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR))};

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);
		return;
	}

	err = bk_sync_init();
	if (err != 0) {
		TEST_FAIL("Failed to initialize backchannel (err %d)", err);
		return;
	}

	err = bt_conn_cb_register(&dut_callbacks);
	__ASSERT_NO_MSG(err == 0);

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		TEST_FAIL("Advertising failed to start (err %d)", err);
		return;
	}

	WAIT_FOR_FLAG(flag_is_connected);

	/* The host auto-initiates a parameter update around 5 seconds after
	 * connecting (CONFIG_BT_GAP_AUTO_UPDATE_CONN_PARAMS). Wait long enough for
	 * it to be sent and rejected by the central, then verify the rejection
	 * callback was NOT invoked: it is only meant for application-initiated
	 * updates.
	 */
	k_sleep(K_SECONDS(8));

	if (IS_FLAG_SET(flag_param_rejected)) {
		TEST_FAIL("Rejection callback fired for a host-initiated update");
		return;
	}

	bk_sync_send();
	TEST_PASS("Host-initiated update rejection did not trigger the callback");
}
