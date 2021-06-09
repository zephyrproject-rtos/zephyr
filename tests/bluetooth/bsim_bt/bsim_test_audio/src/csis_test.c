/*
 * Copyright (c) 2019 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifdef CONFIG_BT_CSIS
#include "../../../../../subsys/bluetooth/host/audio/csis.h"

#include "common.h"

static struct bt_conn_cb conn_callbacks;
extern enum bst_result_t bst_result;
static volatile bool g_locked;
static uint8_t dyn_rank;
static uint8_t sirk_read_req_rsp = BT_CSIS_READ_SIRK_REQ_RSP_ACCEPT;

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}
	printk("Connected\n");
}

static void csis_disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);

	if (reason == BT_HCI_ERR_REMOTE_USER_TERM_CONN) {
		PASS("Client successfully disconnected\n");
	} else {
		FAIL("Client disconnected unexpectectly (0x%02x)\n", reason);
	}
}

static void csis_lock_changed_cb(struct bt_conn *conn, bool locked)
{
	printk("Client %p %s the lock\n", conn, locked ? "locked" : "released");
	g_locked = locked;
}

static uint8_t sirk_read_req_cb(struct bt_conn *conn)
{
	return sirk_read_req_rsp;
}

static struct bt_csis_cb_t csis_cbs = {
	.lock_changed = csis_lock_changed_cb,
	.sirk_read_req = sirk_read_req_cb,
};

static void bt_ready(int err)
{
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Audio Server: Bluetooth initialized\n");

	err = bt_csis_advertise(true);
	if (err) {
		FAIL("Advertising failed to start (err %d)\n", err);
	}
}


static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = csis_disconnected,
};

static void test_main(void)
{
	int err;

	err = bt_enable(bt_ready);

	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);
	bt_csis_register_cb(&csis_cbs);

	if (dyn_rank) {
		printk("Setting rank to %u\n", dyn_rank);
		err = bt_csis_test_set_rank(dyn_rank);
		if (err) {
			FAIL("Could not set rank to %u: %d", dyn_rank, err);
			return;
		}
	}
}

static void test_force_release(void)
{
	int err;

	err = bt_enable(bt_ready);

	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);
	bt_csis_register_cb(&csis_cbs);

	if (dyn_rank) {
		printk("Setting rank to %u\n", dyn_rank);
		err = bt_csis_test_set_rank(dyn_rank);
		if (err) {
			FAIL("Could not set rank to %u: %d", dyn_rank, err);
			return;
		}
	}

	WAIT_FOR(g_locked);
	printk("Force releasing set\n");
	bt_csis_lock(false, true);
}

static void test_csis_enc(void)
{
	printk("Running %s\n", __func__);
	sirk_read_req_rsp = BT_CSIS_READ_SIRK_REQ_RSP_ACCEPT_ENC;
	test_main();
}

static void test_args(int argc, char *argv[])
{
	long rank_arg;

	if (argc) {

		if (!strcmp(argv[0], "rank")) {
			rank_arg = strtol(argv[1], NULL, 10);

			if (rank_arg < 1 || rank_arg > CONFIG_BT_CSIS_SET_SIZE) {
				FAIL("Could not set rank %ld\n", rank_arg);
				return;
			}

			dyn_rank = (uint8_t)rank_arg;
			bs_trace_info_time(1, "Rank arg %u\n", dyn_rank);
		}
	}
}

static const struct bst_test_instance test_connect[] = {
	{
		.test_id = "csis",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
		.test_args_f = test_args,
	},
	{
		.test_id = "csis_release",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_force_release,
		.test_args_f = test_args,
	},
	{
		.test_id = "csis_enc",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_csis_enc,
		.test_args_f = test_args,
	},

	BSTEST_END_MARKER
};

struct bst_test_list *test_csis_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_connect);
}
#else

struct bst_test_list *test_csis_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_CSIS */
