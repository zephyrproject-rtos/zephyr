/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CAP_INITIATOR)

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/cap.h>
#include "common.h"
#include "unicast_common.h"

extern enum bst_result_t bst_result;

CREATE_FLAG(flag_connected);
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
		if (csis_inst == NULL)  {
			FAIL("Failed to discover CAS CSIS");

			return;
		}

		printk("Found CAS with CSIS %p\n", csis_inst);
	} else {
		printk("Found CAS\n");
	}

	SET_FLAG(flag_discovered);
}

static struct bt_cap_initiator_cb cap_cb = {
	.unicast_discovery_complete = cap_discovery_complete_cb
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		bt_conn_unref(default_conn);
		default_conn = NULL;

		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);
	SET_FLAG(flag_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
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

	err = bt_cap_initiator_register_cb(&cap_cb);
	if (err != 0) {
		FAIL("Failed to register CAP callbacks (err %d)\n", err);
		return;
	}
}

static void scan_and_connect(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
	WAIT_FOR_FLAG(flag_connected);
}

static void discover_cas(void)
{
	int err;

	UNSET_FLAG(flag_discovered);

	err = bt_cap_initiator_unicast_discover(default_conn);
	if (err != 0) {
		printk("Failed to discover CAS: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_discovered);
}

static void test_main(void)
{
	init();

	scan_and_connect();

	WAIT_FOR_FLAG(flag_mtu_exchanged);

	discover_cas();

	PASS("CAP initiator passed\n");
}

static const struct bst_test_instance test_cap_initiator[] = {
	{
		.test_id = "cap_initiator",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_cap_initiator_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_cap_initiator);
}

#else /* !(CONFIG_BT_CAP_INITIATOR) */

struct bst_test_list *test_cap_initiator_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_CAP_INITIATOR */
