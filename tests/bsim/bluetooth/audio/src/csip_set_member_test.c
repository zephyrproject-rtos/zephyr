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
struct bt_csip_set_member_register_param param = {
	.set_size = 3,
	.rank = 1,
	.lockable = true,
	/* Using the CSIS test sample SIRK */
	.set_sirk = { 0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce,
		      0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45 },
};

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

	param.cb = &csip_cbs;

	err = bt_csip_set_member_register(&param, &svc_inst);
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

static void test_main(void)
{
	int err;

	err = bt_enable(bt_ready);

	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_connected);
	WAIT_FOR_UNSET_FLAG(flag_connected);

	err = bt_csip_set_member_unregister(svc_inst);
	if (err != 0) {
		FAIL("Could not unregister CSIP (err %d)\n", err);
		return;
	}
	svc_inst = NULL;

	PASS("CSIP Set member passed: Client successfully disconnected\n");
}

static void test_force_release(void)
{
	int err;

	err = bt_enable(bt_ready);

	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_connected);

	WAIT_FOR_COND(g_locked);
	printk("Force releasing set\n");
	bt_csip_set_member_lock(svc_inst, false, true);

	WAIT_FOR_UNSET_FLAG(flag_connected);

	err = bt_csip_set_member_unregister(svc_inst);
	if (err != 0) {
		FAIL("Could not unregister CSIP (err %d)\n", err);
		return;
	}
	svc_inst = NULL;

	PASS("CSIP Set member passed: Client successfully disconnected\n");
}

static void test_csip_enc(void)
{
	printk("Running %s\n", __func__);
	sirk_read_req_rsp = BT_CSIP_READ_SIRK_REQ_RSP_ACCEPT_ENC;
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
		.test_id = "csip_set_member",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
		.test_args_f = test_args,
	},
	{
		.test_id = "csip_set_member_release",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_force_release,
		.test_args_f = test_args,
	},
	{
		.test_id = "csip_set_member_enc",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_csip_enc,
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
