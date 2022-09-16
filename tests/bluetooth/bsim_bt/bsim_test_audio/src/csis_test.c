/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifdef CONFIG_BT_CSIS
#include <zephyr/bluetooth/audio/csis.h>

#include "common.h"

static struct bt_csis *csis;
static struct bt_conn_cb conn_callbacks;
extern enum bst_result_t bst_result;
static volatile bool g_locked;
static uint8_t sirk_read_req_rsp = BT_CSIS_READ_SIRK_REQ_RSP_ACCEPT;
struct bt_csis_register_param param = {
	.set_size = 3,
	.rank = 1,
	.lockable = true,
	/* Using the CSIS test sample SIRK */
	.set_sirk = { 0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce,
		      0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45 },
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
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
		FAIL("Client disconnected unexpectedly (0x%02x)\n", reason);
	}
}

static void csis_lock_changed_cb(struct bt_conn *conn, struct bt_csis *csis,
				 bool locked)
{
	printk("Client %p %s the lock\n", conn, locked ? "locked" : "released");
	g_locked = locked;
}

static uint8_t sirk_read_req_cb(struct bt_conn *conn, struct bt_csis *csis)
{
	return sirk_read_req_rsp;
}

static struct bt_csis_cb csis_cbs = {
	.lock_changed = csis_lock_changed_cb,
	.sirk_read_req = sirk_read_req_cb,
};

static void bt_ready(int err)
{
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Audio Server: Bluetooth initialized\n");

	err = bt_csis_advertise(csis, true);
	if (err != 0) {
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

	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	param.cb = &csis_cbs;

	err = bt_csis_register(&param, &csis);
	if (err != 0) {
		FAIL("Could not register CSIS: %d", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);
}

static void test_force_release(void)
{
	int err;

	err = bt_enable(bt_ready);

	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	param.cb = &csis_cbs;

	err = bt_csis_register(&param, &csis);
	if (err != 0) {
		FAIL("Could not register CSIS: %d", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);

	WAIT_FOR_COND(g_locked);
	printk("Force releasing set\n");
	bt_csis_lock(csis, false, true);
}

static void test_csis_enc(void)
{
	printk("Running %s\n", __func__);
	sirk_read_req_rsp = BT_CSIS_READ_SIRK_REQ_RSP_ACCEPT_ENC;
	test_main();
}

static void test_args(int argc, char *argv[])
{
	for (size_t argn = 0; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (strcmp(arg, "size") == 0) {
			param.set_size = strtol(argv[++argn], NULL, 10);
		} else if (strcmp(arg, "rank") == 0) {
			param.rank = strtol(argv[++argn], NULL, 10);
		} else if (strcmp(arg, "not-lockable") == 0) {
			param.lockable = false;
		} else if (strcmp(arg, "sirk") == 0) {
			size_t len;

			argn++;

			len = hex2bin(argv[argn], strlen(argv[argn]),
				      param.set_sirk, sizeof(param.set_sirk));
			if (len == 0) {
				FAIL("Could not parse SIRK");
				return;
			}
		} else {
			FAIL("Invalid arg: %s", arg);
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
