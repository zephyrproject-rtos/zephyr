/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_BASS
#include <zephyr/bluetooth/audio/bass.h>
#include "common.h"

extern enum bst_result_t bst_result;

static volatile bool g_cb;
static volatile bool g_pa_synced;
static struct bt_conn *g_conn;
static bool g_connected;

static void pa_synced(struct bt_bass_recv_state *recv_state,
		      const struct bt_le_per_adv_sync_synced_info *info)
{
	printk("Receive state %p synced\n", recv_state);
	g_pa_synced = true;
}

static void pa_term(struct bt_bass_recv_state *recv_state,
		    const struct bt_le_per_adv_sync_term_info *info)
{
	printk("Receive state %p sync terminated\n", recv_state);
	g_pa_synced = false;
}

static void pa_recv(struct bt_bass_recv_state *recv_state,
		    const struct bt_le_per_adv_sync_recv_info *info,
		    struct net_buf_simple *buf)
{
	printk("Receive state %p received data\n", recv_state);
}

static struct bt_bass_cb bass_cb = {
	.pa_synced = pa_synced,
	.pa_term = pa_term,
	.pa_recv = pa_recv
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);
	g_conn = conn;
	g_connected = true;
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void test_main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	bt_bass_register_cb(&bass_cb);
	bt_conn_cb_register(&conn_callbacks);

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, AD_SIZE, NULL, 0);
	if (err) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	WAIT_FOR_COND(g_connected);

	WAIT_FOR_COND(g_pa_synced);

	PASS("BASS passed\n");
}

static const struct bst_test_instance test_bass[] = {
	{
		.test_id = "bass",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_bass_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_bass);
}
#else
struct bst_test_list *test_bass_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_BASS */
