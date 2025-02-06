/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/l2cap.h>

#include "host/hci_core.h"
#include "common.h"

#include "babblekit/testcase.h"
#include "babblekit/flags.h"

#define LOG_MODULE_NAME main
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

DEFINE_FLAG_STATIC(is_connected);
DEFINE_FLAG_STATIC(flag_l2cap_connected);
DEFINE_FLAG_STATIC(flag_l2cap_rx_ok);

static struct bt_l2cap_le_chan test_chan;

NET_BUF_POOL_DEFINE(sdu_rx_pool,
		    CONFIG_BT_MAX_CONN, BT_L2CAP_SDU_BUF_SIZE(L2CAP_SDU_LEN),
		    8, NULL);

struct net_buf *alloc_buf_cb(struct bt_l2cap_chan *chan)
{
	return net_buf_alloc(&sdu_rx_pool, K_NO_WAIT);
}

void sent_cb(struct bt_l2cap_chan *chan)
{
	LOG_DBG("%p", chan);
}

int recv_cb(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	static int sdu_count;

	LOG_INF("SDU RX: len %d", buf->len);

	LOG_HEXDUMP_DBG(buf->data, buf->len, "L2CAP RX");
	sdu_count++;

	/* Verify data follows the expected sequence */
	for (int i = 0; i < buf->len; i++) {
		__ASSERT_NO_MSG(buf->data[i] == (uint8_t)i);
	}

	/* We expect two SDUs */
	if (sdu_count == 2) {
		SET_FLAG(flag_l2cap_rx_ok);
	}

	return 0;
}

void l2cap_chan_connected_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct bt_l2cap_le_chan *chan =
		CONTAINER_OF(l2cap_chan, struct bt_l2cap_le_chan, chan);

	SET_FLAG(flag_l2cap_connected);

	LOG_DBG("%x (tx mtu %d mps %d) (rx mtu %d mps %d)",
		l2cap_chan,
		chan->tx.mtu,
		chan->tx.mps,
		chan->rx.mtu,
		chan->rx.mps);
}

void l2cap_chan_disconnected_cb(struct bt_l2cap_chan *chan)
{
	UNSET_FLAG(flag_l2cap_connected);
	LOG_DBG("%p", chan);
}

static struct bt_l2cap_chan_ops ops = {
	.connected = l2cap_chan_connected_cb,
	.disconnected = l2cap_chan_disconnected_cb,
	.alloc_buf = alloc_buf_cb,
	.recv = recv_cb,
	.sent = sent_cb,
};

int server_accept_cb(struct bt_conn *conn, struct bt_l2cap_server *server,
		     struct bt_l2cap_chan **chan)
{
	struct bt_l2cap_le_chan *le_chan = &test_chan;

	memset(le_chan, 0, sizeof(*le_chan));
	le_chan->chan.ops = &ops;
	le_chan->rx.mtu = L2CAP_MTU;
	*chan = &le_chan->chan;

	LOG_DBG("accepting new l2cap connection");

	return 0;
}

static struct bt_l2cap_server test_l2cap_server = {
	.accept = server_accept_cb
};

static int l2cap_server_register(bt_security_t sec_level)
{
	test_l2cap_server.psm = 0;
	test_l2cap_server.sec_level = sec_level;

	int err = bt_l2cap_server_register(&test_l2cap_server);

	TEST_ASSERT(err == 0, "Failed to register l2cap server.");

	return test_l2cap_server.psm;
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		TEST_FAIL("Failed to connect to %s (%u)", addr, conn_err);
		return;
	}

	LOG_DBG("%s", addr);

	SET_FLAG(is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("%p %s (reason 0x%02x)", conn, addr, reason);

	UNSET_FLAG(is_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	struct bt_le_conn_param *param;
	struct bt_conn *conn;
	int err;

	err = bt_le_scan_stop();
	if (err) {
		TEST_FAIL("Stop LE scan failed (err %d)", err);
		return;
	}

	char str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, str, sizeof(str));

	LOG_DBG("Connecting to %s", str);

	param = BT_LE_CONN_PARAM_DEFAULT;
	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &conn);
	if (err) {
		TEST_FAIL("Create conn failed (err %d)", err);
		return;
	}
}

static void connect(void)
{
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_ACTIVE,
		.options = BT_LE_SCAN_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	UNSET_FLAG(is_connected);

	int err = bt_le_scan_start(&scan_param, device_found);

	TEST_ASSERT(!err, "Scanning failed to start (err %d)", err);

	LOG_DBG("Central initiating connection...");
	WAIT_FOR_FLAG(is_connected);
}

static void disconnect_device(struct bt_conn *conn, void *data)
{
	int err;

	SET_FLAG(is_connected);

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(!err, "Failed to initate disconnect (err %d)", err);

	LOG_DBG("Waiting for disconnection...");
	WAIT_FOR_FLAG_UNSET(is_connected);
}

static void do_dlu(struct bt_conn *conn, void *data)
{
	int err;

	struct bt_conn_le_data_len_param param;

	param.tx_max_len = 200;
	param.tx_max_time = 1712;

	err = bt_conn_le_data_len_update(conn, &param);
	TEST_ASSERT(err == 0, "Can't update data length (err %d)", err);
}

void test_procedure_0(void)
{
	LOG_DBG("L2CAP MPS DUT/central started*");
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);
	LOG_DBG("Central Bluetooth initialized.");

	int psm = l2cap_server_register(BT_SECURITY_L1);

	LOG_DBG("Registered server PSM %x", psm);
	__ASSERT_NO_MSG(psm == L2CAP_PSM);

	connect();

	bt_conn_foreach(BT_CONN_TYPE_LE, do_dlu, NULL);

	WAIT_FOR_FLAG(flag_l2cap_rx_ok);

	bt_conn_foreach(BT_CONN_TYPE_LE, disconnect_device, NULL);

	TEST_PASS("DUT done");
}

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "test_0",
		.test_main_f = test_procedure_0,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_to_add);
};

bst_test_install_t test_installers[] = {install, NULL};

int main(void)
{
	bst_main();

	return 0;
}
