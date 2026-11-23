/*
 * Copyright (c) 2022 Codecoup
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/ias.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

#include "bstests.h"
#include "common.h"

LOG_MODULE_REGISTER(ias_client_test);

#ifdef CONFIG_BT_IAS_CLIENT

/* Device numbers of the two clients in test_scripts/ias_two_clients.sh */
#define VALID_LVL_CLIENT_DEV_ID   1U
#define INVALID_LVL_CLIENT_DEV_ID 2U

extern enum bst_result_t bst_result;

CREATE_FLAG(g_service_discovered);
CREATE_FLAG(g_alert_lvl_discovered);
static uint16_t alert_lvl_handle;

static void discover_cb(struct bt_conn *conn, int err)
{
	ARG_UNUSED(conn);

	if (err != 0) {
		FAIL("Failed to discover IAS (err %d)\n", err);
		return;
	}

	LOG_INF("IAS discovered");
	SET_FLAG(g_service_discovered);
}

static const struct bt_ias_client_cb ias_client_cb = {
	.discover = discover_cb,
};

static void test_alert_high(struct bt_conn *conn)
{
	int err;

	err = bt_ias_client_alert_write(conn, BT_IAS_ALERT_LVL_HIGH_ALERT);
	if (err == 0) {
		LOG_INF("High alert sent");
	} else {
		FAIL("Failed to send high alert\n");
	}
}

static void test_alert_mild(struct bt_conn *conn)
{
	int err;

	err = bt_ias_client_alert_write(conn, BT_IAS_ALERT_LVL_MILD_ALERT);
	if (err == 0) {
		LOG_INF("Mild alert sent");
	} else {
		FAIL("Failed to send mild alert\n");
	}
}

static void test_alert_stop(struct bt_conn *conn)
{
	int err;

	err = bt_ias_client_alert_write(conn, BT_IAS_ALERT_LVL_NO_ALERT);
	if (err == 0) {
		LOG_INF("Stop alert sent");
	} else {
		FAIL("Failed to send no alert\n");
	}
}

static void discover_ias(void)
{
	int err;

	UNSET_FLAG(g_service_discovered);

	err = bt_ias_discover(default_conn);
	if (err < 0) {
		FAIL("Failed to discover IAS (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(g_service_discovered);
}

static uint8_t alert_lvl_discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				     struct bt_gatt_discover_params *params)
{
	const struct bt_gatt_chrc *chrc;

	ARGS_UNUSED(conn, params);

	if (attr == NULL) {
		FAIL("Alert Level characteristic not found\n");
		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;
	alert_lvl_handle = chrc->value_handle;
	SET_FLAG(g_alert_lvl_discovered);

	return BT_GATT_ITER_STOP;
}

static void discover_alert_lvl(void)
{
	static struct bt_gatt_discover_params params;
	int err;

	UNSET_FLAG(g_alert_lvl_discovered);

	params.uuid = BT_UUID_ALERT_LEVEL;
	params.func = alert_lvl_discover_cb;
	params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	err = bt_gatt_discover(default_conn, &params);
	if (err != 0) {
		FAIL("Failed to discover the Alert Level characteristic (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(g_alert_lvl_discovered);
}

static void connect_to_server(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	LOG_INF("Bluetooth initialized");

	err = bt_le_scan_cb_register(&common_scan_cb);
	if (err != 0) {
		FAIL("Failed to register scan callbacks (err %d)\n", err);
		return;
	}

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	LOG_INF("Scanning successfully started");

	WAIT_FOR_FLAG(flag_connected);
}

static void test_main(void)
{
	int err;

	connect_to_server();

	err = bt_ias_client_cb_register(&ias_client_cb);
	if (err != 0) {
		FAIL("Failed to register callbacks (err %d)\n", err);
		return;
	}

	discover_ias();
	discover_ias(); /* test that we can discover twice */

	/* Set alert levels with a delay to let the server handle any changes it want */
	test_alert_high(default_conn);
	k_sleep(K_SECONDS(1));

	test_alert_mild(default_conn);
	k_sleep(K_SECONDS(1));

	test_alert_stop(default_conn);
	k_sleep(K_SECONDS(1));

	PASS("IAS client PASS\n");
}

static void test_valid_lvl_client(void)
{
	int err;

	connect_to_server();

	err = bt_ias_client_cb_register(&ias_client_cb);
	if (err != 0) {
		FAIL("Failed to register callbacks (err %d)\n", err);
		return;
	}

	discover_ias();

	test_alert_high(default_conn);

	/* A Write Command gets no response. The server has handled it once it answers the
	 * requests of another discovery.
	 */
	discover_ias();

	/* Let the other client connect and write its invalid level */
	backchannel_sync_send(INVALID_LVL_CLIENT_DEV_ID);
	backchannel_sync_wait(INVALID_LVL_CLIENT_DEV_ID);

	test_alert_mild(default_conn);

	PASS("IAS valid level client PASS\n");
}

static void test_invalid_lvl_client(void)
{
	/* The first value after the Alert Level range */
	const uint8_t invalid_lvl = (uint8_t)BT_IAS_ALERT_LVL_HIGH_ALERT + 1U;
	int err;

	/* The server has the High Alert of the other client by now */
	backchannel_sync_wait(VALID_LVL_CLIENT_DEV_ID);

	connect_to_server();

	discover_alert_lvl();

	err = bt_gatt_write_without_response(default_conn, alert_lvl_handle, &invalid_lvl,
					     sizeof(invalid_lvl), false);
	if (err != 0) {
		FAIL("Failed to send the invalid alert level (err %d)\n", err);
		return;
	}

	/* As for the High Alert of the other client: wait until the server has handled it */
	discover_alert_lvl();

	backchannel_sync_send(VALID_LVL_CLIENT_DEV_ID);

	PASS("IAS invalid level client PASS\n");
}

static const struct bst_test_instance test_ias[] = {
	{
		.test_id = "ias_client",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	{
		.test_id = "ias_client_valid_lvl",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_valid_lvl_client,
	},
	{
		.test_id = "ias_client_invalid_lvl",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_invalid_lvl_client,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_ias_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_ias);
}
#else
struct bst_test_list *test_ias_client_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_IAS_CLIENT */
