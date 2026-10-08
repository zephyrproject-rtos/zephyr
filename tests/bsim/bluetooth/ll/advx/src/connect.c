/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
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

/* The static random identity address of the advertiser, C0:00:00:00:00:01 */
static const bt_addr_le_t adv_addr = {
	.type = BT_ADDR_LE_RANDOM,
	.a = { { 0x01, 0x00, 0x00, 0x00, 0x00, 0xC0 } },
};

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

static struct bt_conn_cb conn_cb = {
	.connected = connected,
	.disconnected = disconnected,
};

static void connection_check(void)
{
	WAIT_FOR_FLAG(flag_connected);

	/* A connection that only one of the two devices has entered ends there
	 * with the Connection Failed to be Established error after 6
	 * connection intervals, as no packet comes from the other one. Wait
	 * well beyond that and check that the connection is still up.
	 */
	(void)k_sleep(K_SECONDS(2));
	TEST_ASSERT(!IS_FLAG_SET(flag_disconnected), "Connection lost");
}

static void test_conn_adv(void)
{
	bt_addr_le_t addr = adv_addr;
	struct bt_le_ext_adv *adv;
	int err;

	TEST_START("conn_adv");

	err = bt_id_create(&addr, NULL);
	TEST_ASSERT(err >= 0, "Identity create failed (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_conn_cb_register(&conn_cb);
	TEST_ASSERT(err == 0, "Callback register failed (err %d)", err);

	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN, NULL, &adv);
	TEST_ASSERT(err == 0, "Advertising set create failed (err %d)", err);

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(err == 0, "Advertising start failed (err %d)", err);

	connection_check();

	TEST_PASS("conn_adv");
}

static void test_conn_init_fal(void)
{
	int err;

	TEST_START("conn_init_fal");

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_conn_cb_register(&conn_cb);
	TEST_ASSERT(err == 0, "Callback register failed (err %d)", err);

	err = bt_le_filter_accept_list_add(&adv_addr);
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
