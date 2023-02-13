/*
 * Copyright (c) 2019 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_MCS

#include <zephyr/bluetooth/audio/media_proxy.h>

#include "common.h"

extern enum bst_result_t bst_result;

CREATE_FLAG(ble_link_is_ready);

/* Callback after Bluetoot initialization attempt */
static void bt_ready(int err)
{
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, AD_SIZE, NULL, 0);
	if (err) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

/* Callback when connected */
static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
	} else {
		default_conn = bt_conn_ref(conn);
		printk("Connected: %s\n", addr);
		SET_FLAG(ble_link_is_ready);
	}
}

static void test_main(void)
{
	int err;
	static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
	};

	printk("Media Control Server test application.  Board: %s\n", CONFIG_BOARD);

	bt_conn_cb_register(&conn_callbacks);

	/* Initialize media player */
	err = media_proxy_pl_init();
	if (err) {
		FAIL("Initializing MPL failed (err %d)", err);
		return;
	}

	/* Initialize Bluetooth, get connected */
	err = bt_enable(bt_ready);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(ble_link_is_ready);

	PASS("MCS passed\n");
}

static const struct bst_test_instance test_mcs[] = {
	{
		.test_id = "mcs",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_mcs_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_mcs);
}

#else

struct bst_test_list *test_mcs_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_MCS */
