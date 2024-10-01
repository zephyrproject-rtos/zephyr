/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"
#include "babblekit/testcase.h"
#include <zephyr/bluetooth/conn.h>

static K_SEM_DEFINE(sem_connected, 0, 1);

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	TEST_ASSERT(conn);
	TEST_ASSERT(err == 0, "Expected success");

	k_sem_give(&sem_connected);
	bt_conn_unref(conn);
}

static struct bt_conn_cb conn_cb = {
	.connected = connected_cb,
};

static void test_peripheral_dummy(void)
{
	int err;
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR))};

	bt_conn_cb_register(&conn_cb);

	bt_addr_le_t addr = {.type = BT_ADDR_LE_RANDOM,
			     .a.val = {0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0}};

	err = bt_id_create(&addr, NULL);
	TEST_ASSERT(err == 0, "Failed to create iD (err %d)", err);

	/* Initialize Bluetooth */
	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	err = bt_le_adv_start(BT_LE_ADV_CONN_ONE_TIME, ad, ARRAY_SIZE(ad), NULL, 0);
	TEST_ASSERT(err == 0, "Advertising failed to start (err %d)", err);

	err = k_sem_take(&sem_connected, K_FOREVER);
	TEST_ASSERT(err == 0, "Failed getting connected timeout", err);

	TEST_PASS("Passed");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "peripheral_dummy",
		.test_descr = "Connectable peripheral",
		.test_tick_f = bst_tick,
		.test_main_f = test_peripheral_dummy,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_peripheral_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
