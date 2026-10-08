/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/kernel.h>

#include "bstests.h"
#include "babblekit/flags.h"
#include "babblekit/testcase.h"

#define ADV_ADDR "R:C0:00:00:00:00:01"

DEFINE_FLAG_STATIC(flag_connected);
DEFINE_FLAG_STATIC(flag_disconnected);

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err == BT_HCI_ERR_SUCCESS) {
		SET_FLAG(flag_connected);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	SET_FLAG(flag_disconnected);
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void connection_check(void)
{
	for (int i = 0; (i < 50) && !IS_FLAG_SET(flag_connected); i++) {
		k_sleep(K_MSEC(100));
	}

	TEST_ASSERT(IS_FLAG_SET(flag_connected), "Not connected");

	/* A peripheral that the central did not see connect loses the
	 * connection when the first connection events do not come.
	 */
	k_sleep(K_SECONDS(2));
	TEST_ASSERT(!IS_FLAG_SET(flag_disconnected), "Connection lost");
}

static void test_conn_adv(void)
{
	struct bt_le_ext_adv *adv;
	bt_addr_le_t addr;
	int err;

	TEST_START("conn_adv");

	err = bt_addr_le_from_str(ADV_ADDR, &addr);
	TEST_ASSERT(err == 0, "Invalid address (err %d)", err);

	err = bt_id_create(&addr, NULL);
	TEST_ASSERT(err >= 0, "Identity create failed (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN, NULL, &adv);
	TEST_ASSERT(err == 0, "Advertising set create failed (err %d)", err);

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(err == 0, "Advertising start failed (err %d)", err);

	connection_check();

	TEST_PASS("conn_adv");
}

static void test_conn_init_fal(void)
{
	bt_addr_le_t addr;
	int err;

	TEST_START("conn_init_fal");

	err = bt_addr_le_from_str(ADV_ADDR, &addr);
	TEST_ASSERT(err == 0, "Invalid address (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_le_filter_accept_list_add(&addr);
	TEST_ASSERT(err == 0, "Filter Accept List add failed (err %d)", err);

	err = bt_conn_le_create_auto(BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT);
	TEST_ASSERT(err == 0, "Connection create failed (err %d)", err);

	connection_check();

	TEST_PASS("conn_init_fal");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "conn_adv",
		.test_descr = "Connectable extended advertising",
		.test_main_f = test_conn_adv,
	},
	{
		.test_id = "conn_init_fal",
		.test_descr = "Initiator with the Filter Accept List",
		.test_main_f = test_conn_init_fal,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_connect_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
