/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2020-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "bstests.h"
#include "common.h"
#ifdef CONFIG_BT_CSIP_SET_MEMBER
static struct bt_csip_set_member_svc_inst *svc_inst;
extern enum bst_result_t bst_result;
static volatile bool g_locked;
static uint8_t sirk_read_req_rsp = BT_CSIP_READ_SIRK_REQ_RSP_ACCEPT;
struct bt_csip_set_member_register_param param = {
	.lockable = true,
	/* Using the CSIS test sample SIRK */
	.sirk = {0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce, 0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d,
		 0x7d, 0x45},
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
	struct bt_le_ext_adv *ext_adv;

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

	err = bt_csip_set_member_generate_rsi(svc_inst, csip_rsi);
	if (err != 0) {
		FAIL("Failed to generate RSI (err %d)\n", err);
		return;
	}

	setup_connectable_adv(&ext_adv);
}

static void test_sirk(void)
{
	const uint8_t new_sirk[] = {0xff, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce,
				    0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45};
	uint8_t tmp_sirk[BT_CSIP_SIRK_SIZE];
	int err;

	printk("Setting new SIRK\n");
	err = bt_csip_set_member_sirk(svc_inst, new_sirk);
	if (err != 0) {
		FAIL("Failed to set SIRK: %d\n", err);
		return;
	}

	printk("Getting new SIRK\n");
	err = bt_csip_set_member_get_sirk(svc_inst, tmp_sirk);
	if (err != 0) {
		FAIL("Failed to get SIRK: %d\n", err);
		return;
	}

	if (memcmp(new_sirk, tmp_sirk, BT_CSIP_SIRK_SIZE) != 0) {
		FAIL("The SIRK set and the SIRK set were different\n");
		return;
	}

	printk("New SIRK correctly set and retrieved\n");
}

static void update_set_size_and_rank(void)
{
	struct bt_csip_set_member_set_info info;
	uint8_t old_set_size;
	uint8_t old_rank;
	uint8_t new_set_size;
	uint8_t new_rank;
	int err;

	err = bt_csip_set_member_get_info(svc_inst, &info);
	if (err != 0) {
		FAIL("Failed to get SIRK: %d\n", err);
		return;
	}

	/* Simulate a new device being added as rank 1 to the set, making the set size increase by 1
	 * and this device's rank increase by 1
	 */
	old_set_size = info.set_size;
	old_rank = info.rank;
	new_set_size = old_set_size + 1U;
	new_rank = old_rank + 1U;

	printk("Setting new SIRK\n");
	err = bt_csip_set_member_set_size_and_rank(svc_inst, new_set_size, new_rank);
	if (err != 0) {
		FAIL("Failed to set new size and rank: %d\n", err);
		return;
	}

	printk("Getting new SIRK\n");
	err = bt_csip_set_member_get_info(svc_inst, &info);
	if (err != 0) {
		FAIL("Failed to get SIRK: %d\n", err);
		return;
	}

	if (info.set_size != new_set_size) {
		FAIL("Unexpected set size %u != %u\n", info.set_size, new_set_size);
		return;
	}

	if (info.rank != new_rank) {
		FAIL("Unexpected rank %u != %u\n", info.rank, new_rank);
		return;
	}

	printk("New size correctly set and retrieved\n");
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

	if (param.lockable) {
		/* Waiting for lock */
		WAIT_FOR_COND(g_locked);
		/* Waiting for lock release */
		WAIT_FOR_COND(!g_locked);
		/* Waiting for lock */
		WAIT_FOR_COND(g_locked);
		/* Waiting for lock release */
		WAIT_FOR_COND(!g_locked);
	}

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

static void test_new_sirk(void)
{
	int err;

	err = bt_enable(bt_ready);

	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_connected);

	backchannel_sync_send_all();
	backchannel_sync_wait_all();

	test_sirk();

	WAIT_FOR_UNSET_FLAG(flag_connected);

	err = bt_csip_set_member_unregister(svc_inst);
	if (err != 0) {
		FAIL("Could not unregister CSIP (err %d)\n", err);
		return;
	}
	svc_inst = NULL;

	PASS("CSIP Set member passed: Client successfully disconnected\n");
}

static void test_new_set_size_and_rank(void)
{
	int err;

	err = bt_enable(bt_ready);

	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_connected);

	backchannel_sync_send_all();
	backchannel_sync_wait_all();

	update_set_size_and_rank();

	WAIT_FOR_UNSET_FLAG(flag_connected);

	err = bt_csip_set_member_unregister(svc_inst);
	if (err != 0) {
		FAIL("Could not unregister CSIP (err %d)\n", err);
		return;
	}
	svc_inst = NULL;

	PASS("CSIP Set member passed: Client successfully disconnected\n");
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

			len = hex2bin(argv[argn], strlen(argv[argn]), param.sirk,
				      sizeof(param.sirk));
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
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
		.test_args_f = test_args,
	},
	{
		.test_id = "csip_set_member_release",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_force_release,
		.test_args_f = test_args,
	},
	{
		.test_id = "csip_set_member_enc",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_csip_enc,
		.test_args_f = test_args,
	},
	{
		.test_id = "csip_set_member_new_sirk",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_new_sirk,
		.test_args_f = test_args,
	},
	{
		.test_id = "csip_set_member_new_size_and_rank",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_new_set_size_and_rank,
		.test_args_f = test_args,
	},
	BSTEST_END_MARKER,
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
