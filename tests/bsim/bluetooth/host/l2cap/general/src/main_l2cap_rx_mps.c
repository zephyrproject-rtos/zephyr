/* main_l2cap_rx_mps.c - Application-provided L2CAP RX MPS */

/*
 * Copyright (c) 2026 Xiaomi Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>

#include <babblekit/flags.h>
#include <babblekit/testcase.h>

#include "bstests.h"

LOG_MODULE_REGISTER(main_l2cap_rx_mps, LOG_LEVEL_INF);

#define PSM_OK		0x0081
#define PSM_BAD_MPS	0x0082

/* The MPS values the central asks for: above the minimum of the
 * connection type, but below MTU + SDU header so that the stack cannot
 * have derived them from the MTU.
 */
#define LEGACY_MPS	25
#define ECRED_MPS	64
/* MTU of the test channels: large enough that the MPS values above are
 * below MTU + 2 (so with the alloc_buf callback set they exercise the
 * application-provided path), small enough to be a valid SDU size.
 */
#define CHAN_MTU	100
/* The MPS of the channels the peripheral hands back to a bad-MPS request. */
#define BAD_MPS		10

static struct bt_conn *default_conn;

DEFINE_FLAG_STATIC(is_connected);
DEFINE_FLAG_STATIC(chan_connected);
DEFINE_FLAG_STATIC(chan_disconnected);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static int recv_cb(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	net_buf_unref(buf);

	return 0;
}

/* No data is exchanged; the callback only opts the channels out of the
 * MTU truncation for channels without reassembly.
 */
NET_BUF_POOL_FIXED_DEFINE(rx_pool, 4,
			  BT_L2CAP_BUF_SIZE(BT_L2CAP_SDU_BUF_SIZE(CHAN_MTU)), 8, NULL);

static struct net_buf *alloc_buf_cb(struct bt_l2cap_chan *chan)
{
	return net_buf_alloc(&rx_pool, K_NO_WAIT);
}

static void connected_cb(struct bt_l2cap_chan *chan)
{
	SET_FLAG(chan_connected);
}

static void disconnected_cb(struct bt_l2cap_chan *chan)
{
	SET_FLAG(chan_disconnected);
}

static const struct bt_l2cap_chan_ops chan_ops = {
	.alloc_buf = alloc_buf_cb,
	.recv = recv_cb,
	.connected = connected_cb,
	.disconnected = disconnected_cb,
};

static void chan_init(struct bt_l2cap_le_chan *le, uint16_t mtu, uint16_t mps)
{
	memset(le, 0, sizeof(*le));
	le->chan.ops = &chan_ops;
	le->rx.mtu = mtu;
	le->rx.mps = mps;
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	if (conn_err != 0) {
		TEST_FAIL("Failed to connect to %s (%u)", bt_conn_dst_str(conn), conn_err);
		return;
	}

	default_conn = bt_conn_ref(conn);

	SET_FLAG(is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (default_conn != conn) {
		TEST_FAIL("Conn mismatch disconnect %s %p)", bt_conn_dst_str(conn), conn);
		return;
	}

	bt_conn_drop(&default_conn);
	UNSET_FLAG(is_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	int err;

	err = bt_le_scan_stop();
	if (err != 0) {
		TEST_FAIL("Stop LE scan failed (err %d)", err);
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &default_conn);
	if (err != 0) {
		TEST_FAIL("Create conn failed (err %d)", err);
		return;
	}
}

static void test_central_main(void)
{
	struct bt_l2cap_chan *ecred_chans[BT_L2CAP_ECRED_CHAN_MAX_PER_REQ] = { NULL };
	struct bt_l2cap_le_chan legacy;
	struct bt_l2cap_le_chan ecred;
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_ACTIVE,
		.options = BT_LE_SCAN_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Can't enable Bluetooth (err %d)", err);
		return;
	}

	err = bt_le_scan_start(&scan_param, device_found);
	if (err != 0) {
		TEST_FAIL("Scanning failed to start (err %d)", err);
		return;
	}

	WAIT_FOR_FLAG(is_connected);

	/* Legacy LE credit based: an MPS below the minimum of 23 is rejected */
	chan_init(&legacy, CHAN_MTU, 22);
	err = bt_l2cap_chan_connect(default_conn, &legacy.chan, PSM_OK);
	TEST_ASSERT(err == -EINVAL, "legacy connect with rx.mps 22 returned %d", err);

	/* Legacy: an MPS within the range is kept, not adjusted */
	chan_init(&legacy, CHAN_MTU, LEGACY_MPS);
	err = bt_l2cap_chan_connect(default_conn, &legacy.chan, PSM_OK);
	TEST_ASSERT(err == 0, "legacy connect returned %d", err);

	WAIT_FOR_FLAG(chan_connected);
	TEST_ASSERT(legacy.rx.mps == LEGACY_MPS, "legacy rx.mps is %u, not %u",
		    legacy.rx.mps, LEGACY_MPS);
	UNSET_FLAG(chan_connected);

	err = bt_l2cap_chan_disconnect(&legacy.chan);
	TEST_ASSERT(err == 0, "legacy disconnect returned %d", err);
	WAIT_FOR_FLAG(chan_disconnected);
	UNSET_FLAG(chan_disconnected);

	/* Enhanced credit based: an MPS below BT_L2CAP_ECRED_MIN_MPS is rejected */
	chan_init(&ecred, CHAN_MTU, ECRED_MPS - 1);
	ecred_chans[0] = &ecred.chan;
	err = bt_l2cap_ecred_chan_connect(default_conn, ecred_chans, PSM_OK);
	TEST_ASSERT(err == -EINVAL, "ecred connect with rx.mps %u returned %d",
		    ECRED_MPS - 1, err);

	/* Enhanced credit based: the minimum is kept, not adjusted */
	chan_init(&ecred, CHAN_MTU, ECRED_MPS);
	err = bt_l2cap_ecred_chan_connect(default_conn, ecred_chans, PSM_OK);
	TEST_ASSERT(err == 0, "ecred connect returned %d", err);

	WAIT_FOR_FLAG(chan_connected);
	TEST_ASSERT(ecred.rx.mps == ECRED_MPS, "ecred rx.mps is %u, not %u",
		    ecred.rx.mps, ECRED_MPS);
	UNSET_FLAG(chan_connected);

	err = bt_l2cap_chan_disconnect(&ecred.chan);
	TEST_ASSERT(err == 0, "ecred disconnect returned %d", err);
	WAIT_FOR_FLAG(chan_disconnected);
	UNSET_FLAG(chan_disconnected);

	/* A server that hands back a channel with an invalid MPS refuses the
	 * connection instead of accepting it with an adjusted value.
	 */
	chan_init(&legacy, CHAN_MTU, LEGACY_MPS);
	err = bt_l2cap_chan_connect(default_conn, &legacy.chan, PSM_BAD_MPS);
	TEST_ASSERT(err == 0, "connect to the bad-MPS server returned %d", err);

	WAIT_FOR_FLAG(chan_disconnected);
	TEST_ASSERT(!IS_FLAG_SET(chan_connected), "the bad-MPS channel connected anyway");

	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err != 0) {
		TEST_FAIL("Disconnection failed (err %d)", err);
		return;
	}

	TEST_PASS("L2CAP RX MPS central Passed");
}

static struct bt_l2cap_le_chan peer_legacy;
static struct bt_l2cap_le_chan peer_ecred;
static struct bt_l2cap_le_chan peer_bad_mps;

static uint16_t observed_tx_mps[2];
static uint8_t observed;
static uint8_t bad_mps_accepts;

static int accept_ok(struct bt_conn *conn, struct bt_l2cap_server *server,
		     struct bt_l2cap_chan **chan)
{
	/* The connections arrive in the order the central makes them: one LE
	 * credit based, then one enhanced credit based.
	 */
	*chan = (observed == 0) ? &peer_legacy.chan : &peer_ecred.chan;

	return 0;
}

static int accept_bad_mps(struct bt_conn *conn, struct bt_l2cap_server *server,
			  struct bt_l2cap_chan **chan)
{
	bad_mps_accepts++;
	*chan = &peer_bad_mps.chan;

	return 0;
}

static struct bt_l2cap_server server_ok = {
	.accept = accept_ok,
	.psm = PSM_OK,
};

static struct bt_l2cap_server server_bad_mps = {
	.accept = accept_bad_mps,
	.psm = PSM_BAD_MPS,
};

static void peer_connected_cb(struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_le_chan *le_chan = BT_L2CAP_LE_CHAN(chan);

	if (observed < ARRAY_SIZE(observed_tx_mps)) {
		observed_tx_mps[observed] = le_chan->tx.mps;
	}
	observed++;

	SET_FLAG(chan_connected);
}

static const struct bt_l2cap_chan_ops peer_chan_ops = {
	.alloc_buf = alloc_buf_cb,
	.recv = recv_cb,
	.connected = peer_connected_cb,
	.disconnected = disconnected_cb,
};

static void test_peripheral_main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Can't enable Bluetooth (err %d)", err);
		return;
	}

	/* The MPS the peripheral receives is the one the central advertised,
	 * so the channels hand their rx endpoint to the stack: only the MTU
	 * is set, the MPS is derived.
	 */
	peer_legacy.chan.ops = &peer_chan_ops;
	peer_legacy.rx.mtu = CHAN_MTU;
	peer_ecred.chan.ops = &peer_chan_ops;
	peer_ecred.rx.mtu = CHAN_MTU;
	peer_bad_mps.chan.ops = &peer_chan_ops;
	peer_bad_mps.rx.mtu = CHAN_MTU;
	peer_bad_mps.rx.mps = BAD_MPS;

	err = bt_l2cap_server_register(&server_ok);
	if (err != 0) {
		TEST_FAIL("Failed to register server (err %d)", err);
		return;
	}

	err = bt_l2cap_server_register(&server_bad_mps);
	if (err != 0) {
		TEST_FAIL("Failed to register bad-MPS server (err %d)", err);
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		TEST_FAIL("Advertising failed to start (err %d)", err);
		return;
	}

	WAIT_FOR_FLAG(is_connected);

	/* One LE credit based and one enhanced credit based connection */
	for (int i = 0; i < 2; i++) {
		WAIT_FOR_FLAG(chan_connected);
		UNSET_FLAG(chan_connected);
		WAIT_FOR_FLAG(chan_disconnected);
		UNSET_FLAG(chan_disconnected);
	}

	WAIT_FOR_FLAG_UNSET(is_connected);

	TEST_ASSERT(observed == 2, "%u channels connected, not 2", observed);
	TEST_ASSERT(observed_tx_mps[0] == LEGACY_MPS,
		    "legacy tx.mps is %u, not %u", observed_tx_mps[0], LEGACY_MPS);
	TEST_ASSERT(observed_tx_mps[1] == ECRED_MPS,
		    "ecred tx.mps is %u, not %u", observed_tx_mps[1], ECRED_MPS);
	TEST_ASSERT(bad_mps_accepts == 1, "%u bad-MPS connections reached accept, not 1",
		    bad_mps_accepts);

	TEST_PASS("L2CAP RX MPS peripheral Passed");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "rx_mps_central",
		.test_descr = "Central L2CAP RX MPS",
		.test_main_f = test_central_main
	},
	{
		.test_id = "rx_mps_peripheral",
		.test_descr = "Peripheral L2CAP RX MPS",
		.test_main_f = test_peripheral_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_main_l2cap_rx_mps_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
