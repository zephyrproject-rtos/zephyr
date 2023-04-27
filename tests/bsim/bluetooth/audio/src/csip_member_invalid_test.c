/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifdef CONFIG_BT_CSIP_SET_MEMBER
#include <zephyr/bluetooth/audio/csip.h>

#include "common.h"

static struct bt_csip_set_member_svc_inst *svc_inst;
extern enum bst_result_t bst_result;
static volatile bool g_locked;
static uint8_t sirk_read_req_rsp = BT_CSIP_READ_SIRK_REQ_RSP_ACCEPT;

enum test_type {
	NON_LOCKABLE,
	NO_RANK,
	NO_SIZE,
	INVALID_SIRK_1,
	INVALID_SIRK_2,
	DISCONNECT,
	VALID,
} invalid_type;

struct bt_csip_set_member_register_param param = {
	.set_size = 3,
	.rank = 1,
	.lockable = true,
	/* Using the CSIS test sample SIRK */
	.set_sirk = { 0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce,
		      0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45 },
};

struct bt_csip_set_member_register_param param_no_rank = {
	.set_size = 3,
	.lockable = true,
	/* Using the CSIS test sample SIRK */
	.set_sirk = { 0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce,
		      0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45 },
};

struct bt_csip_set_member_register_param param_non_lockable = {
	.set_size = 3,
	.rank = 0,
	.lockable = false,
	/* Using the CSIS test sample SIRK */
	.set_sirk = { 0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce,
		      0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45 },
};

struct bt_csip_set_member_register_param param_no_size = {
	.rank = 1,
	.lockable = true,
	/* Using the CSIS test sample SIRK */
	.set_sirk = { 0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce,
		      0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45 },
};

struct bt_csip_set_member_register_param param_invalid_sirk = {
	.set_size = 3,
	.rank = 1,
	.lockable = true,
	/* Using the CSIS test sample invalid SIRK */
	.set_sirk = { 0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce,
		      0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45 },
};

struct bt_csip_set_member_register_param param_invalid_sirk = {
	.set_size = 3,
	.rank = 1,
	.lockable = true,
	/* Using the CSIS test sample invalid SIRK */
	.set_sirk = { 0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce,
		      0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x46 },
};

static void csip_disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);

	if (reason == BT_HCI_ERR_REMOTE_USER_TERM_CONN) {
		PASS("Client successfully disconnected\n");
	} else {
		FAIL("Client disconnected unexpectedly (0x%02x)\n", reason);
	}
}

static void csip_lock_changed_cb(struct bt_conn *conn,
				 struct bt_csip_set_member_svc_inst *svc_inst,
				 bool locked)
{
	printk("Client %p %s the lock\n", conn, locked ? "locked" : "released");
	g_locked = locked;
}

static uint8_t sirk_read_req_cb(struct bt_conn *conn,
				struct bt_csip_set_member_svc_inst *svc_inst)
{
	return sirk_read_req_rsp;
}

static struct bt_csip_set_member_cb csip_cbs = {
	.lock_changed = csip_lock_changed_cb,
	.sirk_read_req = sirk_read_req_cb,
};

static void bt_ready(int err)
{
	uint8_t rsi[BT_CSIP_RSI_SIZE];
	struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
		BT_CSIP_DATA_RSI(rsi),
	};

	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Audio Server: Bluetooth initialized\n");

	switch (invalid_type)
	{
		case NON_LOCKABLE:
			err = bt_csip_set_member_register(&param_non_lockable, &svc_inst);
			break;
		case NO_RANK:
			err = bt_csip_set_member_register(&param_no_rank, &svc_inst);
			break;
		case NO_SIZE:
			err = bt_csip_set_member_register(&param_no_size, &svc_inst);
			break;
		case INVALID_SIRK_1:
			err = bt_csip_set_member_register(&param_invalid_sirk, &svc_inst);
			break;
		case DISCONNECT:
			err = bt_csip_set_member_register(&param, &svc_inst)
		default:
			err = bt_csip_set_member_register(&param, &svc_inst);
	}
	if (err != 0) {
		FAIL("Could not register CSIP (err %d)\n", err);
		return;
	}

	err = bt_csip_set_member_generate_rsi(svc_inst, rsi);
	if (err != 0) {
		FAIL("Failed to generate RSI (err %d)\n", err);
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		FAIL("Advertising failed to start (err %d)\n", err);
	}
}

static struct bt_conn_cb conn_callbacks = {
	.disconnected = csip_disconnected,
};

static void test_main(void)
{
	int err;

	err = bt_enable(bt_ready);

	invalid_type = VALID;

	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);
}

static const struct bst_test_instance test_connect[] = {
	{
		.test_id = "csip_set_member",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
		.test_args_f = test_args,
	},
	{
		.test_id = "csip_set_member_non_lockable",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_non_lockable,
		.test_args_f = test_args,
	},
	{
		.test_id = "csip_set_member_unranked",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_no_rank,
		.test_args_f = test_args,
	},
	{
		.test_id = "csip_set_member_no_size",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_no_size,
		.test_args_f = test_args,
	},
	{
		.test_id = "csip_set_member_invalid_sirk_1",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_invalid_sirk_1,
		.test_args_f = test_args,
	},
	{
		.test_id = "csip_set_member_invalid_sirk_2",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_invalid_sirk_2,
		.test_args_f = test_args,
	},
	{
		.test_id = "csip_set_member_disconnect",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_invalid_disconnect,
		.test_args_f = test_args,
	},
	{
		.test_id = "csip_set_member_reconnect",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_invalid_reconnect,
		.test_args_f = test_args,
	},

	BSTEST_END_MARKER
};

struct bst_test_list *test_csip_set_member_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_connect);
}
#else

struct bst_test_list *test_csip_set_member_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_CSIP_SET_MEMBER */

