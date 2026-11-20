/*
 * Copyright (c) 2022 Codecoup
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/ias.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"

#include "bstests.h"

LOG_MODULE_REGISTER(ias_test);

DEFINE_FLAG_STATIC(g_high_alert_received);
DEFINE_FLAG_STATIC(g_mild_alert_received);
DEFINE_FLAG_STATIC(g_stop_alert_received);
DEFINE_FLAG_STATIC(g_connected);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_IAS_VAL)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void high_alert_cb(void)
{
	SET_FLAG(g_high_alert_received);
}

static void mild_alert_cb(void)
{
	SET_FLAG(g_mild_alert_received);
}

static void no_alert_cb(void)
{
	SET_FLAG(g_stop_alert_received);
}

BT_IAS_CB_DEFINE(ias_callbacks) = {
	.high_alert = high_alert_cb,
	.mild_alert = mild_alert_cb,
	.no_alert = no_alert_cb,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(conn);

	if (err == 0U) {
		SET_FLAG(g_connected);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
};

static void start_adv(void)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	/* The host may already have resumed advertising after a connection */
	TEST_ASSERT(err == 0 || err == -EALREADY, "Failed to start advertising: %d", err);
}

static void test_main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}
	err = bk_sync_init();
	TEST_ASSERT(err == 0, "Failed to init sync: %d", err);

	start_adv();

	WAIT_FOR_FLAG(g_high_alert_received);
	LOG_INF("High alert received");

	err = bt_ias_local_alert_stop();
	TEST_ASSERT(err == 0, "Failed to locally stop alert: %d", err);

	/* Local stop does not trigger callbacks; verify the alert is now off instead */
	err = bt_ias_local_alert_stop();
	TEST_ASSERT(err == -EALREADY, "Alert still active after local stop: %d", err);

	bk_sync_send();
	bk_sync_wait();

	WAIT_FOR_FLAG(g_mild_alert_received);
	LOG_INF("Mild alert received");
	bk_sync_send();
	bk_sync_wait();

	WAIT_FOR_FLAG(g_stop_alert_received);
	LOG_INF("Stop alert received");
	bk_sync_send();
	bk_sync_wait();

	TEST_PASS("IAS test passed\n");
}

static void test_two_clients(void)
{
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	start_adv();
	WAIT_FOR_FLAG(g_connected);
	UNSET_FLAG(g_connected);

	/* The first connection stopped the advertising */
	start_adv();
	WAIT_FOR_FLAG(g_connected);

	WAIT_FOR_FLAG(g_high_alert_received);
	LOG_INF("High alert received");

	/* The first client lowers its level to Mild Alert after the second client wrote an
	 * invalid level, which must not count as a request of any level.
	 */
	while (!IS_FLAG_SET(g_mild_alert_received) && !IS_FLAG_SET(g_stop_alert_received)) {
		k_sleep(K_MSEC(1));
	}
	TEST_ASSERT(!IS_FLAG_SET(g_stop_alert_received),
		    "Alert stopped while a client requests Mild Alert");

	/* Both clients passed before the last write reached this device */
	TEST_PASS_AND_EXIT("IAS two clients test passed\n");
}

static const struct bst_test_instance test_ias[] = {
	{
		.test_id = "ias",
		.test_main_f = test_main,
	},
	{
		.test_id = "ias_two_clients",
		.test_main_f = test_two_clients,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_ias_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_ias);
}
