/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CAP_COMMANDER)

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/sys/byteorder.h>
#include "common.h"
#include "bap_common.h"

extern enum bst_result_t bst_result;

static struct bt_conn *connected_conns[CONFIG_BT_MAX_CONN];
static volatile size_t connected_conn_cnt;

CREATE_FLAG(flag_discovered);
CREATE_FLAG(flag_mtu_exchanged);

static void cap_discovery_complete_cb(struct bt_conn *conn, int err,
				      const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	if (err != 0) {
		FAIL("Failed to discover CAS: %d", err);

		return;
	}

	if (IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER)) {
		if (csis_inst == NULL) {
			FAIL("Failed to discover CAS CSIS");

			return;
		}

		printk("Found CAS with CSIS %p\n", csis_inst);
	} else {
		printk("Found CAS\n");
	}

	SET_FLAG(flag_discovered);
}

static struct bt_cap_commander_cb cap_cb = {
	.discovery_complete = cap_discovery_complete_cb,
};

static void att_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("MTU exchanged\n");
	SET_FLAG(flag_mtu_exchanged);
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = att_mtu_updated,
};

static void init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	bt_gatt_cb_register(&gatt_callbacks);

	err = bt_cap_commander_register_cb(&cap_cb);
	if (err != 0) {
		FAIL("Failed to register CAP callbacks (err %d)\n", err);
		return;
	}
}

static void cap_device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			     struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	struct bt_conn *conn;
	int err;

	/* We're only interested in connectable events */
	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		return;
	}

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, addr);
	if (conn != NULL) {
		/* Already connected to this device */
		bt_conn_unref(conn);
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

	/* connect only to devices in close proximity */
	if (rssi < -70) {
		FAIL("RSSI too low");
		return;
	}

	printk("Stopping scan\n");
	if (bt_le_scan_stop()) {
		FAIL("Could not stop scan");
		return;
	}

	err = bt_conn_le_create(
		addr, BT_CONN_LE_CREATE_CONN,
		BT_LE_CONN_PARAM(BT_GAP_INIT_CONN_INT_MIN, BT_GAP_INIT_CONN_INT_MIN, 0, 400),
		&connected_conns[connected_conn_cnt]);
	if (err) {
		FAIL("Could not connect to peer: %d", err);
	}
}

static void scan_and_connect(void)
{
	int err;

	UNSET_FLAG(flag_connected);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, cap_device_found);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
	WAIT_FOR_FLAG(flag_connected);
	connected_conn_cnt++;
}

static void disconnect_acl(struct bt_conn *conn)
{
	int err;

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err != 0) {
		FAIL("Failed to disconnect (err %d)\n", err);
		return;
	}
}

static void discover_cas(struct bt_conn *conn)
{
	int err;

	UNSET_FLAG(flag_discovered);

	err = bt_cap_commander_discover(conn);
	if (err != 0) {
		printk("Failed to discover CAS: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_discovered);
}

static void test_main_cap_commander_capture_and_render(void)
{
	init();

	/* Connect to and do discovery on all CAP acceptors */
	for (size_t i = 0U; i < get_dev_cnt() - 1; i++) {
		scan_and_connect();

		WAIT_FOR_FLAG(flag_mtu_exchanged);

		/* TODO: We should use CSIP to find set members */
		discover_cas(default_conn);
	}

	/* TODO: Do CAP Commander stuff */

	/* Disconnect all CAP acceptors */
	for (size_t i = 0U; i < connected_conn_cnt; i++) {
		disconnect_acl(connected_conns[i]);
	}
	connected_conn_cnt = 0U;

	PASS("CAP commander capture and rendering passed\n");
}

static const struct bst_test_instance test_cap_commander[] = {
	{
		.test_id = "cap_commander_capture_and_render",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_cap_commander_capture_and_render,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_cap_commander_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_cap_commander);
}

#else /* !CONFIG_BT_CAP_COMMANDER */

struct bst_test_list *test_cap_commander_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_CAP_COMMANDER */
