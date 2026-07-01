/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/l2cap_br.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/ztest.h>

#define LOG_MODULE_NAME test_classic_l2cap_echo_sim
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_TEST_CLASSIC_L2CAP_ECHO_SIM_LOG_LEVEL);

/* Minimal pools, identical to the shell-based BR/EDR test app pattern. */
#define DATA_BREDR_MTU 200
NET_BUF_POOL_FIXED_DEFINE(data_tx_pool, 1, BT_L2CAP_SDU_BUF_SIZE(DATA_BREDR_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

#define TEST_FLAG_DEVICE_FOUND 0
#define TEST_FLAG_CONN_CONNECTED 1

static ATOMIC_DEFINE(test_flags, 32);

static struct bt_conn *br_conn;

static K_SEM_DEFINE(br_connect_changed_sem, 0, 1);

static struct bt_br_discovery_param br_discover_param;
#define BR_DISCOVER_RESULT_COUNT 10
static struct bt_br_discovery_result br_discover_result[BR_DISCOVER_RESULT_COUNT];
static K_SEM_DEFINE(br_discovery_found_sem, 0, 1);

extern bt_addr_t peer_addr;

static void br_connected(struct bt_conn *conn, uint8_t conn_err)
{
	LOG_DBG("connected: conn %p err 0x%02x", (void *)conn, conn_err);
	if (conn_err == 0 && br_conn == NULL) {
		br_conn = bt_conn_ref(conn);
		atomic_set_bit(test_flags, TEST_FLAG_CONN_CONNECTED);
		k_sem_give(&br_connect_changed_sem);
	} else {
		LOG_ERR("Connection failed");
	}
}

static void br_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_DBG("disconnected: conn %p reason 0x%02x", (void *)conn, reason);
	if (br_conn != NULL) {
		bt_conn_unref(br_conn);
		br_conn = NULL;
	}
	atomic_clear_bit(test_flags, TEST_FLAG_CONN_CONNECTED);
	k_sem_give(&br_connect_changed_sem);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = br_connected,
	.disconnected = br_disconnected,
};

static void br_connect_to_peer(void)
{
	struct bt_conn *conn;
	int err;

	if (br_conn != NULL) {
		return;
	}

	if (!atomic_test_bit(test_flags, TEST_FLAG_DEVICE_FOUND)) {
		err = k_sem_take(&br_discovery_found_sem,
				 K_MSEC(ARRAY_SIZE(br_discover_result) * 1280));
		zassert_equal(err, 0, "Failed to found peer device (err %d)", err);
		zassert_true(atomic_test_bit(test_flags, TEST_FLAG_DEVICE_FOUND),
			     "Peer device not found");

		err = bt_br_discovery_stop();
		if ((err != 0) && (err != -EALREADY)) {
			LOG_ERR("Failed to stop GAP discovery procedure (err %d)", err);
		}
	}

	LOG_DBG("Connecting to peer device");
	k_sem_reset(&br_connect_changed_sem);
	conn = bt_conn_create_br(&peer_addr, BT_BR_CONN_PARAM_DEFAULT);
	zassert_true(conn != NULL, "BR connection creating failed");

	err = k_sem_take(&br_connect_changed_sem, K_SECONDS(CONFIG_BT_CREATE_CONN_TIMEOUT));
	zassert_equal(err, 0, "Connection timeout (err %d)", err);
	zassert_true(br_conn != NULL, "Connect handle is NULL");
	zassert_equal(br_conn, conn, "Connect mismatched %p != %p", br_conn, conn);

	LOG_DBG("BR connection established");
	bt_conn_unref(conn);
}

static void br_disconnect_from_peer(void)
{
	int err;

	if (br_conn == NULL) {
		return;
	}

	LOG_DBG("Disconnecting from peer device");
	k_sem_reset(&br_connect_changed_sem);
	err = bt_conn_disconnect(br_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	zassert_equal(err, 0, "Failed to disconnect (err %d)", err);

	err = k_sem_take(&br_connect_changed_sem, K_SECONDS(30));
	zassert_equal(err, 0, "Disconnection timeout (err %d)", err);
	zassert_equal(br_conn, NULL, "br_conn is not NULL after disconnect");
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = br_connected,
	.disconnected = br_disconnected,
};

static void test_wait_for_connect(void)
{
	int err;

	err = k_sem_take(&br_connect_changed_sem, K_SECONDS(CONFIG_BT_CREATE_CONN_TIMEOUT));
	zassert_equal(err, 0, "Connection timeout (err %d)", err);
	zassert_true(atomic_test_bit(test_flags, TEST_FLAG_CONN_CONNECTED), "Connection failed");
}

static void test_wait_for_disconnect(k_timeout_t timeout)
{
	int err;

	err = k_sem_take(&br_connect_changed_sem, timeout);
	zassert_equal(err, 0, "Disconnection timeout (err %d)", err);
	zassert_false(atomic_test_bit(test_flags, TEST_FLAG_CONN_CONNECTED),
		      "Disconnection failed");
}

static K_SEM_DEFINE(echo_req_sem, 0, 1);
static K_SEM_DEFINE(echo_rsp_sem, 0, 1);

static uint8_t last_identifier;

static void echo_req_cb(struct bt_conn *conn, uint8_t identifier, struct net_buf *buf)
{
	LOG_DBG("RX ECHO REQ id=%u", identifier);
	LOG_HEXDUMP_DBG(buf->data, buf->len, "ECHO REQ data:");

	last_identifier = identifier;
	k_sem_give(&echo_req_sem);
}

static void echo_rsp_cb(struct bt_conn *conn, struct net_buf *buf)
{
	LOG_HEXDUMP_DBG(buf->data, buf->len, "ECHO RSP data:");
	k_sem_give(&echo_rsp_sem);
}

static struct bt_l2cap_br_echo_cb echo_cb = {
	.req = echo_req_cb,
	.rsp = echo_rsp_cb,
};

static void l2cap_echo_send_req(void)
{
	struct net_buf *buf;
	int err;
	int retries = 3;

	buf = net_buf_alloc(&data_tx_pool, K_FOREVER);
	zassert_not_null(buf, "Unable to allocate Tx buffer");

	net_buf_reserve(buf, BT_L2CAP_BR_ECHO_REQ_RESERVE);
	net_buf_add_mem(buf, "Hello Peer", 10);
	do {
		err = bt_l2cap_br_echo_req(br_conn, buf);
		if (err == -EBUSY) {
			k_sleep(K_SECONDS(1));
		}
		retries--;
	} while (retries > 0 && err == -EBUSY);
	zassert_equal(err, 0, "Failed to send ECHO REQ (err %d)", err);
}

static void l2cap_echo_send_rsp(uint8_t identifier)
{
	struct net_buf *buf;
	int err;

	buf = net_buf_alloc(&data_tx_pool, K_FOREVER);
	zassert_not_null(buf, "Unable to allocate Tx buffer");

	net_buf_reserve(buf, BT_L2CAP_BR_ECHO_REQ_RESERVE);
	net_buf_add_mem(buf, "Hello Peer", 10);
	err = bt_l2cap_br_echo_rsp(br_conn, identifier, buf);
	zassert_equal(err, 0, "Failed to send ECHO RSP (err %d)", err);
}


ZTEST(l2cap_echo_device1, test_01_send_echo_req_and_recv_rsp)
{
	struct net_buf *buf;
	int err;

	err = bt_l2cap_br_echo_cb_register(&echo_cb);
	zassert_equal(err, 0, "echo cb register failed (err %d)", err);

	br_connect_to_peer();
	zassert_true(br_conn != NULL, "Connect handle is NULL");

	l2cap_echo_send_req();
	err = k_sem_take(&echo_rsp_sem, K_SECONDS(5));
	zassert_equal(err, 0, "echo rsp timeout (err %d)", err);

	l2cap_echo_send_req();
	err = k_sem_take(&echo_rsp_sem, K_SECONDS(5));
	zassert_true(err < 0, "Unexpected ECHO RSP (err %d)", err);

	err = k_sem_take(&echo_req_sem, K_SECONDS(30));
	zassert_equal(err, 0, "echo rsp timeout (err %d)", err);

	l2cap_echo_send_rsp(last_identifier);

	err = k_sem_take(&echo_req_sem, K_SECONDS(30));
	zassert_equal(err, 0, "echo rsp timeout (err %d)", err);

	buf = net_buf_alloc(&data_tx_pool, K_FOREVER);
	zassert_not_null(buf, "Unable to allocate Tx buffer");

	err = bt_l2cap_br_echo_rsp(br_conn, 0, buf);
	zassert_equal(err, -EINVAL, "Unexpected return code %d", err);

	net_buf_reserve(buf, BT_L2CAP_BR_ECHO_REQ_RESERVE);
	net_buf_add_mem(buf, "Hello Echo", 10);
	err = bt_l2cap_br_echo_rsp(br_conn, 0, buf);
	zassert_equal(err, -EINVAL, "Unexpected return code %d", err);

	err = bt_l2cap_br_echo_rsp(br_conn, last_identifier + 1, buf);
	zassert_equal(err, 0, "Failed to send ECHO RSP (err %d)", err);

	test_wait_for_disconnect(K_SECONDS(30));
}

ZTEST(l2cap_echo_device2, test_01_send_echo_req_and_recv_rsp)
{
	struct net_buf *buf;
	int err;

	err = bt_l2cap_br_echo_cb_register(&echo_cb);
	zassert_equal(err, 0, "echo cb register failed (err %d)", err);

	test_wait_for_connect();

	err = k_sem_take(&echo_req_sem, K_SECONDS(30));
	zassert_equal(err, 0, "echo rsp timeout (err %d)", err);

	l2cap_echo_send_rsp(last_identifier);

	err = k_sem_take(&echo_req_sem, K_SECONDS(30));
	zassert_equal(err, 0, "echo rsp timeout (err %d)", err);

	buf = net_buf_alloc(&data_tx_pool, K_FOREVER);
	zassert_not_null(buf, "Unable to allocate Tx buffer");

	err = bt_l2cap_br_echo_rsp(br_conn, 0, buf);
	zassert_equal(err, -EINVAL, "Unexpected return code %d", err);

	net_buf_reserve(buf, BT_L2CAP_BR_ECHO_REQ_RESERVE);
	net_buf_add_mem(buf, "Hello Echo", 10);
	err = bt_l2cap_br_echo_rsp(br_conn, 0, buf);
	zassert_equal(err, -EINVAL, "Unexpected return code %d", err);

	err = bt_l2cap_br_echo_rsp(br_conn, last_identifier + 1, buf);
	zassert_equal(err, 0, "Failed to send ECHO RSP (err %d)", err);

	l2cap_echo_send_req();
	err = k_sem_take(&echo_rsp_sem, K_SECONDS(5));
	zassert_equal(err, 0, "echo rsp timeout (err %d)", err);

	l2cap_echo_send_req();
	err = k_sem_take(&echo_rsp_sem, K_SECONDS(5));
	zassert_true(err < 0, "Unexpected ECHO RSP (err %d)", err);

	br_disconnect_from_peer();
}

static void br_discover_timeout(const struct bt_br_discovery_result *results, size_t count)
{
	LOG_DBG("BR discovery done, found %zu devices", count);
}

static void br_discover_recv(const struct bt_br_discovery_result *result)
{
	char br_addr[BT_ADDR_STR_LEN];

	bt_addr_to_str(&result->addr, br_addr, sizeof(br_addr));

	LOG_DBG("[DEVICE]: %s, RSSI %i, COD %u", br_addr, result->rssi, sys_get_le24(result->cod));

	if (!bt_addr_eq(&peer_addr, &result->addr)) {
		return;
	}

	LOG_DBG("  Target %s is found", br_addr);
	atomic_set_bit(test_flags, TEST_FLAG_DEVICE_FOUND);
	k_sem_give(&br_discovery_found_sem);
}

static struct bt_br_discovery_cb br_discover = {
	.recv = br_discover_recv,
	.timeout = br_discover_timeout,
};

static void *setup(void)
{
	int err;

	err = bt_enable(NULL);
	zassert_equal(err, 0, "bt_enable failed (err %d)", err);

	LOG_DBG("Bluetooth initialized");

	err = bt_br_set_connectable(true, NULL);
	zassert_equal(err, 0, "Failed to set connectable (err %d)", err);

	err = bt_br_set_discoverable(true, true);
	zassert_equal(err, 0, "Failed to set discoverable (err %d)", err);

	LOG_DBG("Register discovery callback");
	bt_br_discovery_cb_register(&br_discover);

	br_discover_param.length = ARRAY_SIZE(br_discover_result);
	br_discover_param.limited = false;

	memset(br_discover_result, 0, sizeof(br_discover_result));

	LOG_DBG("Starting Bluetooth inquiry");
	err = bt_br_discovery_start(&br_discover_param, br_discover_result,
				    ARRAY_SIZE(br_discover_result));
	zassert_equal(err, 0, "Bluetooth inquiry failed (err %d)", err);

	return NULL;
}

static void teardown(void *f)
{
	int err;

	LOG_DBG("Disabling Bluetooth");

	/* De-initialize the Bluetooth Subsystem */
	err = bt_disable();
	zassert_equal(err, 0, "Bluetooth de-init failed (err %d)", err);

	LOG_DBG("Bluetooth de-initialized");
}

static void before(void *f)
{
	atomic_clear(test_flags);

	k_sem_reset(&br_connect_changed_sem);
	k_sem_reset(&br_discovery_found_sem);

	k_sem_reset(&echo_req_sem);
	k_sem_reset(&echo_rsp_sem);
	last_identifier = 0;
}

static void after(void *f)
{
	br_disconnect_from_peer();
}

ZTEST_SUITE(l2cap_echo_device1, NULL, setup, before, after, teardown);
ZTEST_SUITE(l2cap_echo_device2, NULL, setup, before, after, teardown);
