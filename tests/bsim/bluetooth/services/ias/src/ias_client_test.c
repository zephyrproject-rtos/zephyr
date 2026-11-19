/*
 * Copyright (c) 2022 Codecoup
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
#include <zephyr/toolchain.h>

#include "testlib/conn.h"
#include "testlib/scan.h"

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"
#include "bstests.h"

LOG_MODULE_REGISTER(ias_client_test);

DEFINE_FLAG_STATIC(g_service_discovered);
DEFINE_FLAG_STATIC(g_alert_lvl_discovered);
static uint16_t alert_lvl_handle;

static void discover_cb(struct bt_conn *conn, int err)
{
	ARG_UNUSED(conn);

	if (err != 0) {
		TEST_FAIL("Failed to discover IAS (err %d)\n", err);
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
		TEST_FAIL("Failed to send high alert\n");
	}
}

static void test_alert_mild(struct bt_conn *conn)
{
	int err;

	err = bt_ias_client_alert_write(conn, BT_IAS_ALERT_LVL_MILD_ALERT);
	if (err == 0) {
		LOG_INF("Mild alert sent");
	} else {
		TEST_FAIL("Failed to send mild alert\n");
	}
}

static void test_alert_stop(struct bt_conn *conn)
{
	int err;

	err = bt_ias_client_alert_write(conn, BT_IAS_ALERT_LVL_NO_ALERT);
	if (err == 0) {
		LOG_INF("Stop alert sent");
	} else {
		TEST_FAIL("Failed to send no alert\n");
	}
}

static void discover_ias(struct bt_conn *conn)
{
	int err;

	UNSET_FLAG(g_service_discovered);

	err = bt_ias_discover(conn);
	if (err < 0) {
		TEST_FAIL("Failed to discover IAS (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(g_service_discovered);
}

static uint8_t alert_lvl_discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				     struct bt_gatt_discover_params *params)
{
	const struct bt_gatt_chrc *chrc;

	ARG_UNUSED(conn);
	ARG_UNUSED(params);

	if (attr == NULL) {
		TEST_FAIL("Alert Level characteristic not found\n");
		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;
	alert_lvl_handle = chrc->value_handle;
	SET_FLAG(g_alert_lvl_discovered);

	return BT_GATT_ITER_STOP;
}

static void discover_alert_lvl(struct bt_conn *conn)
{
	static struct bt_gatt_discover_params params;
	int err;

	UNSET_FLAG(g_alert_lvl_discovered);

	params.uuid = BT_UUID_ALERT_LEVEL;
	params.func = alert_lvl_discover_cb;
	params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	err = bt_gatt_discover(conn, &params);
	TEST_ASSERT(err == 0, "Failed to discover the Alert Level characteristic (err %d)", err);

	WAIT_FOR_FLAG(g_alert_lvl_discovered);
}

static struct bt_conn *connect_to_server(void)
{
	struct bt_conn *conn = NULL;
	bt_addr_le_t addr;
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	LOG_INF("Bluetooth initialized");

	err = bt_testlib_scan_find_name(&addr, CONFIG_BT_DEVICE_NAME);
	TEST_ASSERT(err == 0, "Failed to find IAS server: %d", err);

	err = bt_testlib_connect(&addr, &conn);
	TEST_ASSERT(err == 0, "Failed to connect: %d", err);

	return conn;
}

static void test_main(void)
{
	struct bt_conn *client_conn;
	int err;

	err = bk_sync_init();
	TEST_ASSERT(err == 0, "Failed to init sync: %d", err);

	client_conn = connect_to_server();

	err = bt_ias_client_cb_register(&ias_client_cb);
	TEST_ASSERT(err == 0, "Failed to register callbacks (err %d)", err);

	discover_ias(client_conn);
	discover_ias(client_conn); /* test that we can discover twice */

	/* Sync with the server after each write so it can check every alert level */
	test_alert_high(client_conn);
	bk_sync_send();
	bk_sync_wait();

	test_alert_mild(client_conn);
	bk_sync_send();
	bk_sync_wait();

	test_alert_stop(client_conn);
	bk_sync_send();
	bk_sync_wait();

	TEST_PASS("IAS client PASS\n");
}

/* In tests_scripts/ias_two_clients.sh the two clients are devices 0 and 1, the pair that
 * babblekit sync connects, and the server is device 2.
 */
static void test_valid_lvl_client(void)
{
	struct bt_conn *conn;
	int err;

	err = bk_sync_init();
	TEST_ASSERT(err == 0, "Failed to init sync: %d", err);

	conn = connect_to_server();

	err = bt_ias_client_cb_register(&ias_client_cb);
	TEST_ASSERT(err == 0, "Failed to register callbacks (err %d)", err);

	discover_ias(conn);

	test_alert_high(conn);

	/* A Write Command gets no response. The server has handled it once it answers the
	 * requests of another discovery.
	 */
	discover_ias(conn);

	/* Let the other client connect and write its invalid level */
	bk_sync_send();
	bk_sync_wait();

	test_alert_mild(conn);

	TEST_PASS("IAS valid level client PASS\n");
}

static void test_invalid_lvl_client(void)
{
	/* The first value after the Alert Level range */
	const uint8_t invalid_lvl = (uint8_t)BT_IAS_ALERT_LVL_HIGH_ALERT + 1U;
	struct bt_conn *conn;
	int err;

	err = bk_sync_init();
	TEST_ASSERT(err == 0, "Failed to init sync: %d", err);

	/* The server has the High Alert of the other client by now */
	bk_sync_wait();

	conn = connect_to_server();

	discover_alert_lvl(conn);

	err = bt_gatt_write_without_response(conn, alert_lvl_handle, &invalid_lvl,
					     sizeof(invalid_lvl), false);
	TEST_ASSERT(err == 0, "Failed to send the invalid alert level (err %d)", err);

	/* As for the High Alert of the other client: wait until the server has handled it */
	discover_alert_lvl(conn);

	bk_sync_send();

	TEST_PASS("IAS invalid level client PASS\n");
}

static const struct bst_test_instance test_ias[] = {
	{
		.test_id = "ias_client",
		.test_main_f = test_main,
	},
	{
		.test_id = "ias_client_valid_lvl",
		.test_main_f = test_valid_lvl_client,
	},
	{
		.test_id = "ias_client_invalid_lvl",
		.test_main_f = test_invalid_lvl_client,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_ias_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_ias);
}
