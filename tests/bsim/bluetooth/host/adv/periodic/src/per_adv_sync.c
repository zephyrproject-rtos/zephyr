/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"
#include "common.h"

extern enum bst_result_t bst_result;

static struct bt_conn *g_conn;
static bt_addr_le_t per_addr;
static uint8_t per_sid;

DEFINE_FLAG_STATIC(flag_connected);
DEFINE_FLAG_STATIC(flag_bonded);
DEFINE_FLAG_STATIC(flag_per_adv);
DEFINE_FLAG_STATIC(flag_per_adv_sync);
DEFINE_FLAG_STATIC(flag_per_adv_sync_lost);
DEFINE_FLAG_STATIC(flag_per_adv_recv);

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != BT_HCI_ERR_SUCCESS) {
		TEST_FAIL("Failed to connect to %s: %u", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);
	g_conn = bt_conn_ref(conn);
	SET_FLAG(flag_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason %u)\n", addr, reason);

	bt_conn_unref(g_conn);
	g_conn = NULL;
}

static struct bt_conn_cb conn_cbs = {
	.connected = connected,
	.disconnected = disconnected,
};

static void pairing_complete_cb(struct bt_conn *conn, bool bonded)
{
	if (conn == g_conn && bonded) {
		SET_FLAG(flag_bonded);
	}
}

static struct bt_conn_auth_info_cb auto_info_cbs = {
	.pairing_complete = pairing_complete_cb,
};

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
	if (!IS_FLAG_SET(flag_connected) &&
	    info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) {
		int err;

		printk("Stopping scan\n");
		err = bt_le_scan_stop();
		if (err != 0) {
			TEST_FAIL("Failed to stop scan: %d", err);
			return;
		}

		err = bt_conn_le_create(info->addr, BT_CONN_LE_CREATE_CONN,
					BT_LE_CONN_PARAM_DEFAULT, &g_conn);
		if (err != 0) {
			TEST_FAIL("Could not connect to peer: %d", err);
			return;
		}
	} else if (!IS_FLAG_SET(flag_per_adv) && info->interval != 0U) {

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

static void recv_cb(struct bt_le_per_adv_sync *recv_sync,
		    const struct bt_le_per_adv_sync_recv_info *info,
		    struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	uint8_t buf_data_len;

	if (IS_FLAG_SET(flag_per_adv_recv)) {
		return;
	}

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s advertisement received\n",
	       bt_le_per_adv_sync_get_index(recv_sync), le_addr);

	while (buf->len > 0) {
		buf_data_len = (uint8_t)net_buf_simple_pull_le16(buf);
		if (buf->data[0] - 1 != sizeof(mfg_data) ||
			memcmp(buf->data, mfg_data, sizeof(mfg_data))) {
			TEST_FAIL("Unexpected adv data received");
		}
		net_buf_simple_pull(buf, ARRAY_SIZE(mfg_data));
	}

	SET_FLAG(flag_per_adv_recv);
}

static struct bt_le_per_adv_sync_cb sync_callbacks = {
	.synced = sync_cb,
	.term = term_cb,
	.recv = recv_cb,
};

static void common_init(void)
{
	int err;

	err = bt_enable(NULL);

	if (err) {
		TEST_FAIL("Bluetooth init failed: %d", err);
		return;
	}

	bt_le_scan_cb_register(&scan_callbacks);
	bt_le_per_adv_sync_cb_register(&sync_callbacks);
	bt_conn_cb_register(&conn_cbs);
	bt_conn_auth_info_cb_register(&auto_info_cbs);
}

static void start_scan(void)
{
	int err;

	printk("Start scanning...");

	err = bt_le_scan_start(IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED) ?
			       BT_LE_SCAN_CODED_ACTIVE : BT_LE_SCAN_ACTIVE,
			       NULL);
	if (err) {
		TEST_FAIL("Failed to start scan: %d", err);
		return;
	}
	printk("done.\n");
}

static void create_pa_sync(struct bt_le_per_adv_sync **sync)
{
	struct bt_le_per_adv_sync_param sync_create_param = { 0 };
	int err;

	printk("Creating periodic advertising sync...");
	bt_addr_le_copy(&sync_create_param.addr, &per_addr);
	sync_create_param.options = 0;
	sync_create_param.sid = per_sid;
	sync_create_param.skip = 0;
	sync_create_param.timeout = 0x0a;
	err = bt_le_per_adv_sync_create(&sync_create_param, sync);
	if (err) {
		TEST_FAIL("Failed to create periodic advertising sync: %d", err);
		return;
	}
	printk("done.\n");

	printk("Waiting for periodic sync...\n");
	WAIT_FOR_FLAG(flag_per_adv_sync);
	printk("Periodic sync established.\n");
}

static void start_bonding(void)
{
	int err;

	printk("Setting security...");
	err = bt_conn_set_security(g_conn, BT_SECURITY_L2);
	if (err) {
		TEST_FAIL("Failed to set security: %d", err);
		return;
	}
	printk("done.\n");
}

static void main_per_adv_sync(void)
{
	struct bt_le_per_adv_sync *sync = NULL;

	common_init();
	start_scan();

	printk("Waiting for periodic advertising...\n");
	WAIT_FOR_FLAG(flag_per_adv);
	printk("Found periodic advertising.\n");

	create_pa_sync(&sync);

	printk("Waiting for periodic sync lost...\n");
	WAIT_FOR_FLAG(flag_per_adv_sync_lost);

	TEST_PASS("Periodic advertising sync passed");
}

static void main_per_adv_sync_app_not_scanning(void)
{
	int err;
	struct bt_le_per_adv_sync *sync = NULL;

	common_init();
	start_scan();

	printk("Waiting for periodic advertising...\n");
	WAIT_FOR_FLAG(flag_per_adv);
	printk("Found periodic advertising.\n");

	printk("Stopping scan\n");
	err = bt_le_scan_stop();
	if (err != 0) {
		TEST_FAIL("Failed to stop scan: %d", err);
		return;
	}

	create_pa_sync(&sync);

	printk("Waiting for periodic sync lost...\n");
	WAIT_FOR_FLAG(flag_per_adv_sync_lost);

	TEST_PASS("Periodic advertising sync passed");
}

static void main_per_adv_conn_sync(void)
{
	struct bt_le_per_adv_sync *sync = NULL;

	common_init();
	start_scan();

	printk("Waiting for connection...");
	WAIT_FOR_FLAG(flag_connected);
	printk("done.\n");

	start_scan();

	printk("Waiting for periodic advertising...\n");
	WAIT_FOR_FLAG(flag_per_adv);
	printk("Found periodic advertising.\n");

	create_pa_sync(&sync);

	printk("Waiting for periodic sync lost...\n");
	WAIT_FOR_FLAG(flag_per_adv_sync_lost);

	TEST_PASS("Periodic advertising sync passed");
}

static void main_per_adv_conn_privacy_sync(void)
{
	struct bt_le_per_adv_sync *sync = NULL;

	common_init();
	start_scan();

	printk("Waiting for connection...");
	WAIT_FOR_FLAG(flag_connected);
	printk("done.\n");

	start_bonding();

	printk("Waiting for bonding...");
	WAIT_FOR_FLAG(flag_bonded);
	printk("done.\n");

	start_scan();

	printk("Waiting for periodic advertising...\n");
	WAIT_FOR_FLAG(flag_per_adv);
	printk("Found periodic advertising.\n");

	create_pa_sync(&sync);

	printk("Waiting for periodic sync lost...\n");
	WAIT_FOR_FLAG(flag_per_adv_sync_lost);

	TEST_PASS("Periodic advertising sync passed");
}

static void main_per_adv_long_data_sync(void)
{
#if (CONFIG_BT_PER_ADV_SYNC_BUF_SIZE > 0)
	struct bt_le_per_adv_sync *sync = NULL;

	common_init();
	start_scan();

	printk("Waiting for periodic advertising...\n");
	WAIT_FOR_FLAG(flag_per_adv);
	printk("Found periodic advertising.\n");

	create_pa_sync(&sync);

	printk("Waiting to receive periodic advertisement...\n");
	WAIT_FOR_FLAG(flag_per_adv_recv);

	printk("Waiting for periodic sync lost...\n");
	WAIT_FOR_FLAG(flag_per_adv_sync_lost);
#endif
	TEST_PASS("Periodic advertising long data sync passed");
}

static const struct bst_test_instance per_adv_sync[] = {
	{
		.test_id = "per_adv_sync",
		.test_descr = "Basic periodic advertising sync test. "
			      "Will just sync to a periodic advertiser.",
		.test_main_f = main_per_adv_sync
	},
	{
		.test_id = "per_adv_sync_app_not_scanning",
		.test_descr = "Basic periodic advertising sync test but where "
			      "the app stopped scanning before creating sync."
			      "Expect the host to start scanning automatically.",
		.test_main_f = main_per_adv_sync_app_not_scanning
	},
	{
		.test_id = "per_adv_conn_sync",
		.test_descr = "Periodic advertising sync test, but where there "
			      "is a connection between the advertiser and the "
			      "synchronized device.",
		.test_main_f = main_per_adv_conn_sync
	},
	{
		.test_id = "per_adv_conn_privacy_sync",
		.test_descr = "Periodic advertising sync test, but where "
			      "advertiser and synchronized device are bonded and using  "
			      "privacy",
		.test_main_f = main_per_adv_conn_privacy_sync
	},
	{
		.test_id = "per_adv_long_data_sync",
		.test_descr = "Periodic advertising sync test with larger "
			      "data length. Test is used to verify that "
			      "reassembly of long data is handeled correctly.",
		.test_main_f = main_per_adv_long_data_sync
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_per_adv_sync(struct bst_test_list *tests)
{
	return bst_add_tests(tests, per_adv_sync);
}
