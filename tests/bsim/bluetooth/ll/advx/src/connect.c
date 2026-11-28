/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>

#include "bstests.h"
#include "babblekit/flags.h"
#include "babblekit/testcase.h"
#include "testlib/conn.h"

/* The static random identity address of the advertiser, C0:00:00:00:00:01 */
static const bt_addr_le_t adv_addr = {
	.type = BT_ADDR_LE_RANDOM,
	.a = { { 0x01, 0x00, 0x00, 0x00, 0x00, 0xC0 } },
};

/* The static random identity address of the initiator, C0:00:00:00:00:02 */
static const bt_addr_le_t init_addr = {
	.type = BT_ADDR_LE_RANDOM,
	.a = {{0x02, 0x00, 0x00, 0x00, 0x00, 0xC0}},
};

DEFINE_FLAG_STATIC(flag_connected);
DEFINE_FLAG_STATIC(flag_disconnected);

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err == BT_HCI_ERR_SUCCESS) {
		SET_FLAG(flag_connected);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	SET_FLAG(flag_disconnected);
}

static struct bt_conn_cb conn_cb = {
	.connected = connected,
	.disconnected = disconnected,
};

static void connection_check(void)
{
	WAIT_FOR_FLAG(flag_connected);

	/* A connection that only one of the two devices has entered ends there
	 * with the Connection Failed to be Established error after 6
	 * connection intervals, as no packet comes from the other one. Wait
	 * well beyond that and check that the connection is still up.
	 */
	(void)k_sleep(K_SECONDS(2));
	TEST_ASSERT(!IS_FLAG_SET(flag_disconnected), "Connection lost");
}

static void test_conn_adv(void)
{
	bt_addr_le_t addr = adv_addr;
	struct bt_le_ext_adv *adv;
	int err;

	TEST_START("conn_adv");

	err = bt_id_create(&addr, NULL);
	TEST_ASSERT(err >= 0, "Identity create failed (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_conn_cb_register(&conn_cb);
	TEST_ASSERT(err == 0, "Callback register failed (err %d)", err);

	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN, NULL, &adv);
	TEST_ASSERT(err == 0, "Advertising set create failed (err %d)", err);

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(err == 0, "Advertising start failed (err %d)", err);

	connection_check();

	TEST_PASS("conn_adv");
}

static void test_conn_adv_delayed(void)
{
	/* Start advertising only after the initiator has set up its connection */
	(void)k_sleep(K_SECONDS(1));

	test_conn_adv();
}

static void test_conn_init(void)
{
	struct bt_conn *conn = NULL;
	int err;

	TEST_START("conn_init");

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_conn_cb_register(&conn_cb);
	TEST_ASSERT(err == 0, "Callback register failed (err %d)", err);

	err = bt_testlib_connect(&adv_addr, &conn);
	TEST_ASSERT(err == 0, "Connection failed (err %d)", err);

	connection_check();

	bt_testlib_conn_unref(&conn);

	TEST_PASS("conn_init");
}

static void test_conn_init_fal(void)
{
	int err;

	TEST_START("conn_init_fal");

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_conn_cb_register(&conn_cb);
	TEST_ASSERT(err == 0, "Callback register failed (err %d)", err);

	err = bt_le_filter_accept_list_add(&adv_addr);
	TEST_ASSERT(err == 0, "Filter Accept List add failed (err %d)", err);

	err = bt_conn_le_create_auto(BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT);
	TEST_ASSERT(err == 0, "Connection create failed (err %d)", err);

	connection_check();

	TEST_PASS("conn_init_fal");
}

static void bt_up(void)
{
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_conn_cb_register(&conn_cb);
	TEST_ASSERT(err == 0, "Callback register failed (err %d)", err);
}

static void init_fal_adv(const struct bt_conn_le_create_param *create_param)
{
	struct bt_le_ext_adv *adv;
	int err;

	/* An advertising set of our own, which does not use the Filter Accept List. Enabling any
	 * set updates the controller filters; a non-connectable one needs no connection object
	 * besides the one of the pending connection.
	 */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, NULL, &adv);
	TEST_ASSERT(err == 0, "Advertising set create failed (err %d)", err);

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(err == 0, "Advertising start failed (err %d)", err);

	/* Pause advertising around the Filter Accept List write, and enable it again while the
	 * connection to an accept listed device is pending.
	 */
	err = bt_le_ext_adv_stop(adv);
	TEST_ASSERT(err == 0, "Advertising stop failed (err %d)", err);

	err = bt_le_filter_accept_list_add(&adv_addr);
	TEST_ASSERT(err == 0, "Filter Accept List add failed (err %d)", err);

	err = bt_conn_le_create_auto(create_param, BT_LE_CONN_PARAM_DEFAULT);
	TEST_ASSERT(err == 0, "Connection create failed (err %d)", err);

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(err == 0, "Advertising restart failed (err %d)", err);

	connection_check();
}

static void test_conn_init_fal_adv(void)
{
	TEST_START("conn_init_fal_adv");

	bt_up();
	init_fal_adv(BT_CONN_LE_CREATE_CONN);

	TEST_PASS("conn_init_fal_adv");
}

static void test_conn_init_fal_adv_coded(void)
{
	/* Initiate on the LE Coded PHY only, which uses the coded scan set alone */
	const struct bt_conn_le_create_param create_param =
		BT_CONN_LE_CREATE_PARAM_INIT(BT_CONN_LE_OPT_CODED | BT_CONN_LE_OPT_NO_1M,
					     BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_INTERVAL);

	TEST_START("conn_init_fal_adv_coded");

	bt_up();
	init_fal_adv(&create_param);

	TEST_PASS("conn_init_fal_adv_coded");
}

static void test_conn_adv_coded_delayed(void)
{
	const struct bt_le_adv_param param = BT_LE_ADV_PARAM_INIT(
		BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_CODED,
		BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL);
	bt_addr_le_t addr = adv_addr;
	struct bt_le_ext_adv *adv;
	int err;

	TEST_START("conn_adv_coded");

	err = bt_id_create(&addr, NULL);
	TEST_ASSERT(err >= 0, "Identity create failed (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_conn_cb_register(&conn_cb);
	TEST_ASSERT(err == 0, "Callback register failed (err %d)", err);

	err = bt_le_ext_adv_create(&param, NULL, &adv);
	TEST_ASSERT(err == 0, "Advertising set create failed (err %d)", err);

	/* Start advertising only after the initiator has set up its connection */
	(void)k_sleep(K_SECONDS(1));

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(err == 0, "Advertising start failed (err %d)", err);

	connection_check();

	TEST_PASS("conn_adv_coded");
}

static void adv_fal_enable(void)
{
	bt_addr_le_t addr = adv_addr;
	int err;

	err = bt_id_create(&addr, NULL);
	TEST_ASSERT(err >= 0, "Identity create failed (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_conn_cb_register(&conn_cb);
	TEST_ASSERT(err == 0, "Callback register failed (err %d)", err);

	err = bt_le_filter_accept_list_add(&init_addr);
	TEST_ASSERT(err == 0, "Filter Accept List add failed (err %d)", err);
}

static void adv_fal_start(void)
{
	/* Accept connection requests only from devices on the Filter Accept List */
	const struct bt_le_adv_param param = BT_LE_ADV_PARAM_INIT(
		BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_FILTER_CONN,
		BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL);
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_le_ext_adv_create(&param, NULL, &adv);
	TEST_ASSERT(err == 0, "Advertising set create failed (err %d)", err);

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(err == 0, "Advertising start failed (err %d)", err);
}

static void nconn_adv_start(void)
{
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, NULL, &adv);
	TEST_ASSERT(err == 0, "Advertising set create failed (err %d)", err);

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(err == 0, "Advertising start failed (err %d)", err);
}

static void scan_start(void)
{
	int err;

	/* A scan that does not use the Filter Accept List */
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	TEST_ASSERT(err == 0, "Scan start failed (err %d)", err);
}

static void test_conn_adv_fal_scan(void)
{
	TEST_START("conn_adv_fal_scan");

	adv_fal_enable();
	adv_fal_start();
	scan_start();

	connection_check();

	TEST_PASS("conn_adv_fal_scan");
}

static void test_conn_adv_fal_adv(void)
{
	TEST_START("conn_adv_fal_adv");

	adv_fal_enable();
	adv_fal_start();

	/* A second advertising set, which does not use the Filter Accept List */
	nconn_adv_start();

	connection_check();

	TEST_PASS("conn_adv_fal_adv");
}

static void test_conn_adv1_fal_scan(void)
{
	struct bt_le_ext_adv *adv;
	int err;

	TEST_START("conn_adv1_fal_scan");

	adv_fal_enable();

	/* Take the first advertising set, so that the one using the list is not set 0 */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, NULL, &adv);
	TEST_ASSERT(err == 0, "Advertising set create failed (err %d)", err);

	adv_fal_start();
	scan_start();

	connection_check();

	TEST_PASS("conn_adv1_fal_scan");
}

static void test_conn_init_id_delayed(void)
{
	bt_addr_le_t addr = init_addr;
	struct bt_conn *conn = NULL;
	int err;

	TEST_START("conn_init_id_delayed");

	err = bt_id_create(&addr, NULL);
	TEST_ASSERT(err >= 0, "Identity create failed (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_conn_cb_register(&conn_cb);
	TEST_ASSERT(err == 0, "Callback register failed (err %d)", err);

	/* Connect only after the advertiser has enabled its scan */
	(void)k_sleep(K_SECONDS(1));

	err = bt_testlib_connect(&adv_addr, &conn);
	TEST_ASSERT(err == 0, "Connection failed (err %d)", err);

	connection_check();

	bt_testlib_conn_unref(&conn);

	TEST_PASS("conn_init_id_delayed");
}

static atomic_t id_addr_reports;

static void scan_recv(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
		      struct net_buf_simple *ad)
{
	if (bt_addr_le_eq(addr, &adv_addr)) {
		(void)atomic_inc(&id_addr_reports);
	}
}

static void rl_add_network_privacy(const bt_addr_le_t *peer)
{
	struct bt_hci_cp_le_add_dev_to_rl *cp;
	struct net_buf *buf;
	int err;

	/* A peer with an IRK, in the default Network Privacy mode, so that the controller must not
	 * report its identity address
	 */
	buf = bt_hci_cmd_alloc(K_FOREVER);
	TEST_ASSERT(buf != NULL, "HCI command alloc failed");

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_le_copy(&cp->peer_id_addr, peer);
	(void)memset(cp->peer_irk, 0x11, sizeof(cp->peer_irk));
	(void)memset(cp->local_irk, 0, sizeof(cp->local_irk));

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_ADD_DEV_TO_RL, buf, NULL);
	TEST_ASSERT(err == 0, "Resolving list add failed (err %d)", err);

	buf = bt_hci_cmd_alloc(K_FOREVER);
	TEST_ASSERT(buf != NULL, "HCI command alloc failed");

	net_buf_add_u8(buf, BT_HCI_ADDR_RES_ENABLE);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_ADDR_RES_ENABLE, buf, NULL);
	TEST_ASSERT(err == 0, "Address resolution enable failed (err %d)", err);
}

static void test_scan_rl_adv(void)
{
	int err;

	TEST_START("scan_rl_adv");

	bt_up();
	rl_add_network_privacy(&adv_addr);

	/* Report every advertising PDU, so that no report is held back as a duplicate */
	err = bt_le_scan_start(BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_PASSIVE, BT_LE_SCAN_OPT_NONE,
						BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_WINDOW),
			       scan_recv);
	TEST_ASSERT(err == 0, "Scan start failed (err %d)", err);

	(void)k_sleep(K_SECONDS(2));
	TEST_ASSERT(atomic_get(&id_addr_reports) == 0, "%ld reports of the identity address",
		    atomic_get(&id_addr_reports));

	/* An advertising set of our own, enabled while scanning */
	nconn_adv_start();

	(void)k_sleep(K_SECONDS(2));
	TEST_ASSERT(atomic_get(&id_addr_reports) == 0,
		    "%ld reports of the identity address after enabling advertising",
		    atomic_get(&id_addr_reports));

	TEST_PASS("scan_rl_adv");
}

static void test_nconn_adv_id(void)
{
	bt_addr_le_t addr = adv_addr;
	int err;

	TEST_START("nconn_adv_id");

	err = bt_id_create(&addr, NULL);
	TEST_ASSERT(err >= 0, "Identity create failed (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	/* Legacy advertising from the identity address, rather than from a non-resolvable private
	 * address, which the scanner checks against its resolving list on the primary channel
	 */
	err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, NULL, 0, NULL, 0);
	TEST_ASSERT(err == 0, "Advertising start failed (err %d)", err);

	TEST_PASS("nconn_adv_id");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "conn_adv",
		.test_descr = "Connectable extended advertising",
		.test_main_f = test_conn_adv,
	},
	{
		.test_id = "conn_adv_delayed",
		.test_descr = "Connectable extended advertising started after one second",
		.test_main_f = test_conn_adv_delayed,
	},
	{
		.test_id = "conn_init",
		.test_descr = "Initiator",
		.test_main_f = test_conn_init,
	},
	{
		.test_id = "conn_init_fal",
		.test_descr = "Initiator with the Filter Accept List",
		.test_main_f = test_conn_init_fal,
	},
	{
		.test_id = "conn_init_fal_adv",
		.test_descr = "Initiator with the Filter Accept List that enables an advertising "
			      "set while connecting",
		.test_main_f = test_conn_init_fal_adv,
	},
	{
		.test_id = "conn_adv_fal_scan",
		.test_descr = "Connectable extended advertising with the Filter Accept List that "
			      "enables a scan",
		.test_main_f = test_conn_adv_fal_scan,
	},
	{
		.test_id = "conn_adv_fal_adv",
		.test_descr = "Connectable extended advertising with the Filter Accept List that "
			      "enables a second advertising set",
		.test_main_f = test_conn_adv_fal_adv,
	},
	{
		.test_id = "conn_adv1_fal_scan",
		.test_descr =
			"Connectable extended advertising with the Filter Accept List on a set "
			"other than the first, that enables a scan",
		.test_main_f = test_conn_adv1_fal_scan,
	},
	{
		.test_id = "conn_adv_coded_delayed",
		.test_descr =
			"Connectable extended advertising on LE Coded started after one second",
		.test_main_f = test_conn_adv_coded_delayed,
	},
	{
		.test_id = "conn_init_fal_adv_coded",
		.test_descr =
			"Initiator on LE Coded only with the Filter Accept List that enables an "
			"advertising set while connecting",
		.test_main_f = test_conn_init_fal_adv_coded,
	},
	{
		.test_id = "conn_init_id_delayed",
		.test_descr = "Initiator with a fixed identity that connects after one second",
		.test_main_f = test_conn_init_id_delayed,
	},
	{
		.test_id = "scan_rl_adv",
		.test_descr =
			"Scanner with a Network Privacy peer in the resolving list that enables "
			"advertising, and must not report the peer's identity address",
		.test_main_f = test_scan_rl_adv,
	},
	{
		.test_id = "nconn_adv_id",
		.test_descr = "Non-connectable legacy advertiser using its identity address",
		.test_main_f = test_nconn_adv_id,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_connect_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
