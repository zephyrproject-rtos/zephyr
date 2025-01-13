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

DEFINE_FLAG_STATIC(flag_connected);
DEFINE_FLAG_STATIC(flag_bonded);

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

static void common_init(void)
{
	int err;

	err = bt_enable(NULL);

	if (err) {
		TEST_FAIL("Bluetooth init failed: %d", err);
		return;
	}
	printk("Bluetooth initialized\n");

	bt_conn_cb_register(&conn_cbs);
	bt_conn_auth_info_cb_register(&auto_info_cbs);
}

static void create_per_adv_set(struct bt_le_ext_adv **adv)
{
	int err;

	printk("Creating extended advertising set...");
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, NULL, adv);
	if (err) {
		printk("Failed to create advertising set: %d\n", err);
		return;
	}
	printk("done.\n");

	printk("Setting periodic advertising parameters...");
	err = bt_le_per_adv_set_param(*adv, BT_LE_PER_ADV_DEFAULT);
	if (err) {
		printk("Failed to set periodic advertising parameters: %d\n",
		       err);
		return;
	}
	printk("done.\n");
}

#if defined(CONFIG_BT_CTLR_PHY_CODED)
static void create_per_adv_set_coded(struct bt_le_ext_adv **adv)
{
	int err;

	printk("Creating coded PHY extended advertising set...");
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CODED_NCONN, NULL, adv);
	if (err) {
		printk("Failed to create advertising set: %d\n", err);
		return;
	}
	printk("done.\n");

	printk("Setting periodic advertising parameters...");
	err = bt_le_per_adv_set_param(*adv, BT_LE_PER_ADV_DEFAULT);
	if (err) {
		printk("Failed to set periodic advertising parameters: %d\n",
		       err);
		return;
	}
	printk("done.\n");
}
#endif /* CONFIG_BT_CTLR_PHY_CODED */

static void create_conn_adv_set(struct bt_le_ext_adv **adv)
{
	int err;

	printk("Creating connectable extended advertising set...");
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN, NULL, adv);
	if (err) {
		printk("Failed to create advertising set: %d\n", err);
		return;
	}
	printk("done.\n");
}

static void start_ext_adv_set(struct bt_le_ext_adv *adv)
{
	int err;

	printk("Starting Extended Advertising...");
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising: %d\n", err);
		return;
	}
	printk("done.\n");
}

static void start_per_adv_set(struct bt_le_ext_adv *adv)
{
	int err;

	printk("Starting periodic advertising...");
	err = bt_le_per_adv_start(adv);
	if (err) {
		printk("Failed to start periodic advertising: %d\n", err);
		return;
	}
	printk("done.\n");
}

#if defined(CONFIG_BT_PER_ADV)
static void set_per_adv_data(struct bt_le_ext_adv *adv)
{
	int err;
	const struct bt_data ad[] = {
		BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, ARRAY_SIZE(mfg_data)),
		BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, ARRAY_SIZE(mfg_data))};

	printk("Setting Periodic Advertising Data...");
	err = bt_le_per_adv_set_data(adv, ad, ARRAY_SIZE(ad));
	if (err) {
		printk("Failed to set periodic advertising data: %d\n",
		       err);
		return;
	}
	printk("done.\n");
}
#endif

static void stop_ext_adv_set(struct bt_le_ext_adv *adv)
{
	int err;

	printk("Stopping Extended Advertising...");
	err = bt_le_ext_adv_stop(adv);
	if (err) {
		printk("Failed to stop extended advertising: %d\n",
		       err);
		return;
	}
	printk("done.\n");
}

static void stop_per_adv_set(struct bt_le_ext_adv *adv)
{
	int err;

	printk("Stopping Periodic Advertising...");
	err = bt_le_per_adv_stop(adv);
	if (err) {
		printk("Failed to stop periodic advertising: %d\n",
		       err);
		return;
	}
	printk("done.\n");
}

static void delete_adv_set(struct bt_le_ext_adv *adv)
{
	int err;

	printk("Delete extended advertising set...");
	err = bt_le_ext_adv_delete(adv);
	if (err) {
		printk("Failed Delete extended advertising set: %d\n", err);
		return;
	}
	printk("done.\n");
}

static void main_per_adv_advertiser(void)
{
	struct bt_le_ext_adv *per_adv;

	common_init();

	create_per_adv_set(&per_adv);

	start_per_adv_set(per_adv);
	start_ext_adv_set(per_adv);

	/* Advertise for a bit */
	k_sleep(K_SECONDS(10));

	stop_per_adv_set(per_adv);
	stop_ext_adv_set(per_adv);

	delete_adv_set(per_adv);
	per_adv = NULL;

	TEST_PASS("Periodic advertiser passed");
}

#if defined(CONFIG_BT_CTLR_PHY_CODED)
static void main_per_adv_advertiser_coded(void)
{
	struct bt_le_ext_adv *per_adv;

	common_init();

	create_per_adv_set_coded(&per_adv);

	start_per_adv_set(per_adv);
	start_ext_adv_set(per_adv);

	/* Advertise for a bit */
	k_sleep(K_SECONDS(10));

	stop_per_adv_set(per_adv);
	stop_ext_adv_set(per_adv);

	delete_adv_set(per_adv);
	per_adv = NULL;

	TEST_PASS("Periodic advertiser coded PHY passed");
}
#endif /* CONFIG_BT_CTLR_PHY_CODED */

static void main_per_adv_conn_advertiser(void)
{
	struct bt_le_ext_adv *conn_adv;
	struct bt_le_ext_adv *per_adv;

	common_init();

	create_per_adv_set(&per_adv);
	create_conn_adv_set(&conn_adv);

	start_per_adv_set(per_adv);
	start_ext_adv_set(per_adv);
	start_ext_adv_set(conn_adv);

	WAIT_FOR_FLAG(flag_connected);

	/* Advertise for a bit */
	k_sleep(K_SECONDS(10));

	stop_per_adv_set(per_adv);
	stop_ext_adv_set(per_adv);
	stop_ext_adv_set(conn_adv);

	delete_adv_set(per_adv);
	per_adv = NULL;
	delete_adv_set(conn_adv);
	conn_adv = NULL;

	TEST_PASS("Periodic advertiser passed");
}

static void main_per_adv_conn_privacy_advertiser(void)
{
	struct bt_le_ext_adv *conn_adv;
	struct bt_le_ext_adv *per_adv;

	common_init();

	create_conn_adv_set(&conn_adv);

	start_ext_adv_set(conn_adv);

	WAIT_FOR_FLAG(flag_connected);
	WAIT_FOR_FLAG(flag_bonded);

	/* Start periodic advertising after bonding so that the scanner gets
	 * the resolved address
	 */
	create_per_adv_set(&per_adv);
	start_per_adv_set(per_adv);
	start_ext_adv_set(per_adv);

	/* Advertise for a bit */
	k_sleep(K_SECONDS(10));

	stop_per_adv_set(per_adv);
	stop_ext_adv_set(per_adv);
	stop_ext_adv_set(conn_adv);

	delete_adv_set(per_adv);
	per_adv = NULL;
	delete_adv_set(conn_adv);
	conn_adv = NULL;

	TEST_PASS("Periodic advertiser passed");
}

static void main_per_adv_long_data_advertiser(void)
{
#if defined(CONFIG_BT_PER_ADV)
	struct bt_le_ext_adv *per_adv;

	common_init();

	create_per_adv_set(&per_adv);

	set_per_adv_data(per_adv);
	start_per_adv_set(per_adv);
	start_ext_adv_set(per_adv);

	/* Advertise for a bit */
	k_sleep(K_SECONDS(10));

	stop_per_adv_set(per_adv);
	stop_ext_adv_set(per_adv);

	delete_adv_set(per_adv);
	per_adv = NULL;
#endif
	TEST_PASS("Periodic long data advertiser passed");
}

static const struct bst_test_instance per_adv_advertiser[] = {
	{
		.test_id = "per_adv_advertiser",
		.test_descr = "Basic periodic advertising test. "
			      "Will just start periodic advertising.",
		.test_main_f = main_per_adv_advertiser
	},
#if defined(CONFIG_BT_CTLR_PHY_CODED)
	{
		.test_id = "per_adv_advertiser_coded_phy",
		.test_descr = "Basic periodic advertising test on Coded PHY. "
			      "Advertiser and periodic advertiser uses Coded PHY",
		.test_main_f = main_per_adv_advertiser_coded
	},
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	{
		.test_id = "per_adv_conn_advertiser",
		.test_descr = "Periodic advertising test with concurrent ACL "
			      "and PA sync.",
		.test_main_f = main_per_adv_conn_advertiser
	},
	{
		.test_id = "per_adv_conn_privacy_advertiser",
		.test_descr = "Periodic advertising test with concurrent ACL "
			      "with bonding and PA sync.",
		.test_main_f = main_per_adv_conn_privacy_advertiser
	},
	{
		.test_id = "per_adv_long_data_advertiser",
		.test_descr = "Periodic advertising test with a longer data length. "
			      "To test the reassembly of large data packets",
		.test_main_f = main_per_adv_long_data_advertiser
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_per_adv_advertiser(struct bst_test_list *tests)
{
	return bst_add_tests(tests, per_adv_advertiser);
}
