/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"

#include <zephyr/types.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>

#include "common.h"

extern enum bst_result_t bst_result;

static bt_addr_le_t per_addr;
static uint8_t per_sid;

CREATE_FLAG(flag_per_adv);
CREATE_FLAG(flag_per_adv_sync);
CREATE_FLAG(flag_per_adv_sync_lost);

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
	if (!TEST_FLAG(flag_per_adv) && info->interval != 0U) {

		per_sid = info->sid;
		bt_addr_le_copy(&per_addr, info->addr);

		SET_FLAG(flag_per_adv);
	}
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static void sync_cb(struct bt_le_per_adv_sync *sync,
		    struct bt_le_per_adv_sync_synced_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s synced, "
	       "Interval 0x%04x (%u us)\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr,
	       info->interval, BT_CONN_INTERVAL_TO_US(info->interval));

	SET_FLAG(flag_per_adv_sync);
}

static void term_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s sync terminated\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr);

	SET_FLAG(flag_per_adv_sync_lost);
}

static struct bt_le_per_adv_sync_cb sync_callbacks = {
	.synced = sync_cb,
	.term = term_cb,
};

static void main_per_adv_syncer(void)
{
	struct bt_le_per_adv_sync_param sync_create_param = { 0 };
	struct bt_le_per_adv_sync *sync = NULL;
	int err;

	err = bt_enable(NULL);

	if (err) {
		FAIL("Bluetooth init failed: %d\n", err);
		return;
	}

	bt_le_scan_cb_register(&scan_callbacks);
	bt_le_per_adv_sync_cb_register(&sync_callbacks);

	printk("Start scanning...");
	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	if (err) {
		printk("Failed to start scan: %d\n", err);
		return;
	}
	printk("done.\n");

	printk("Waiting for periodic advertising...\n");
	WAIT_FOR_FLAG(flag_per_adv);
	printk("Found periodic advertising.\n");

	printk("Creating periodic advertising sync...");
	bt_addr_le_copy(&sync_create_param.addr, &per_addr);
	sync_create_param.options = 0;
	sync_create_param.sid = per_sid;
	sync_create_param.skip = 0;
	sync_create_param.timeout = 0x0a;
	err = bt_le_per_adv_sync_create(&sync_create_param, &sync);
	if (err) {
		printk("Failed to create periodic advertising sync: %d\n",
		       err);
		return;
	}
	printk("done.\n");

	printk("Waiting for periodic sync...\n");
	WAIT_FOR_FLAG(flag_per_adv_sync);
	printk("Periodic sync established.\n");

	printk("Waiting for periodic sync lost...\n");
	WAIT_FOR_FLAG(flag_per_adv_sync_lost);

	PASS("Periodic advertising syncer passed\n");
}

static const struct bst_test_instance per_adv_syncer[] = {
	{
		.test_id = "per_adv_syncer",
		.test_descr = "Basic periodic advertising sync test. "
			      "Will just sync to a periodic advertiser.",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = main_per_adv_syncer
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_per_adv_syncer(struct bst_test_list *tests)
{
	return bst_add_tests(tests, per_adv_syncer);
}
