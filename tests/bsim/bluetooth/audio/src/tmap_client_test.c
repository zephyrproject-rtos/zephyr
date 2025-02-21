/*
 * Copyright 2023 NXP
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/tmap.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>
#include <zephyr/sys/byteorder.h>

#include "bstests.h"
#include "common.h"

#ifdef CONFIG_BT_TMAP
extern enum bst_result_t bst_result;

CREATE_FLAG(flag_tmap_discovered);

void tmap_discovery_complete_cb(enum bt_tmap_role role, struct bt_conn *conn, int err)
{
	printk("TMAS discovery done\n");
	SET_FLAG(flag_tmap_discovered);
}

static struct bt_tmap_cb tmap_callbacks = {
	.discovery_complete = tmap_discovery_complete_cb
};

static bool check_audio_support_and_connect(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = user_data;
	struct net_buf_simple tmas_svc_data;
	const struct bt_uuid *uuid;
	uint16_t uuid_val;
	uint16_t peer_tmap_role = 0;
	int err;

	printk("[AD]: %u data_len %u\n", data->type, data->data_len);

	if (data->type != BT_DATA_SVC_DATA16) {
		return true; /* Continue parsing to next AD data type */
	}

	if (data->data_len < sizeof(uuid_val)) {
		printk("AD invalid size %u\n", data->data_len);
		return true; /* Continue parsing to next AD data type */
	}

	net_buf_simple_init_with_data(&tmas_svc_data, (void *)data->data, data->data_len);
	uuid_val = net_buf_simple_pull_le16(&tmas_svc_data);
	uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(uuid_val));
	if (bt_uuid_cmp(uuid, BT_UUID_TMAS) != 0) {
		/* We are looking for the TMAS service data */
		return true; /* Continue parsing to next AD data type */
	}

	printk("Found TMAS in peer adv data!\n");
	if (tmas_svc_data.len < sizeof(peer_tmap_role)) {
		printk("AD invalid size %u\n", data->data_len);
		return false; /* Stop parsing */
	}

	peer_tmap_role = net_buf_simple_pull_le16(&tmas_svc_data);
	if (!(peer_tmap_role & BT_TMAP_ROLE_UMR)) {
		printk("No TMAS UMR support!\n");
		return false; /* Stop parsing */
	}

	printk("Attempt to connect!\n");
	err = bt_le_scan_stop();
	if (err != 0) {
		printk("Failed to stop scan: %d\n", err);
		return false;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT,
				&default_conn);
	if (err != 0) {
		printk("Create conn to failed (%u)\n", err);
		bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	}

	return false; /* Stop parsing */
}

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	printk("SCAN RCV CB\n");

	/* Check for connectable, extended advertising */
	if (((info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) != 0) ||
		((info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE)) != 0) {
		bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
		printk("[DEVICE]: %s, ", le_addr);
		/* Check for TMAS support in advertising data */
		bt_data_parse(buf, check_audio_support_and_connect, (void *)info->addr);
	}
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static void discover_tmas(void)
{
	int err;

	UNSET_FLAG(flag_tmap_discovered);

	/* Discover TMAS service on peer */
	err = bt_tmap_discover(default_conn, &tmap_callbacks);
	if (err != 0) {
		FAIL("Failed to initiate TMAS discovery: %d\n", err);
		return;
	}

	printk("TMAP Central Starting Service Discovery...\n");
	WAIT_FOR_FLAG(flag_tmap_discovered);
}

static void test_main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");
	/* Initialize TMAP */
	err = bt_tmap_register(BT_TMAP_ROLE_CG | BT_TMAP_ROLE_UMS);
	if (err != 0) {
		FAIL("Failed to register TMAP (err %d)\n", err);
		return;
	}

	printk("TMAP initialized. Start scanning...\n");
	/* Scan for peer */
	bt_le_scan_cb_register(&scan_callbacks);
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
	WAIT_FOR_FLAG(flag_connected);

	discover_tmas();
	discover_tmas(); /* test that we can discover twice */

	PASS("TMAP Client test passed\n");
}

static const struct bst_test_instance test_tmap_client[] = {
	{
		.test_id = "tmap_client",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_tmap_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_tmap_client);
}
#else
struct bst_test_list *test_tmap_client_install(struct bst_test_list *tests)
{
	return tests;
}
#endif /* CONFIG_BT_TMAP */
