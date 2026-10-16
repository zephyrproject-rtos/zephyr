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

#define LOG_MODULE_NAME test_classic_l2cap_connless_sim
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_TEST_CLASSIC_L2CAP_CONNLESS_SIM_LOG_LEVEL);

#define TEST_CONNLESS_PSM        0x1001
#define TEST_CONNLESS_PSM_OTHER  0x1003

#define DATA_BREDR_MTU 200
NET_BUF_POOL_FIXED_DEFINE(data_tx_pool, 4, BT_L2CAP_SDU_BUF_SIZE(DATA_BREDR_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

#define TEST_FLAG_DEVICE_FOUND    0
#define TEST_FLAG_CONN_CONNECTED  1

static ATOMIC_DEFINE(test_flags, 32);

static struct bt_conn *br_conn;

static K_SEM_DEFINE(br_connect_changed_sem, 0, 1);

static struct bt_br_discovery_param br_discover_param;
#define BR_DISCOVER_RESULT_COUNT 10
static struct bt_br_discovery_result br_discover_result[BR_DISCOVER_RESULT_COUNT];
static K_SEM_DEFINE(br_discovery_found_sem, 0, 1);

extern bt_addr_t peer_addr;
static struct _test_conn_less_recv_cb {
	atomic_val_t psm[1];
	atomic_val_t last_recv_len[1];
	struct k_sem sem;
} test_conn_less_recv_cb[3];

enum {
	TEST_CONNLESS_CB_IDX = 0,
	TEST_CONNLESS_CB_OTHER_IDX = 1,
	TEST_CONNLESS_CB_ANY = 2,
	TEST_CONNLESS_CB_MAX = TEST_CONNLESS_CB_ANY,
};

static void init_connless_recv_cb(int index)
{
	if (index > TEST_CONNLESS_CB_MAX) {
		zassert_true(false, "Invalid callback index %d", index);
		return;
	}

	k_sem_init(&test_conn_less_recv_cb[index].sem, 0, 1);
	atomic_clear(test_conn_less_recv_cb[index].psm);
	atomic_clear(test_conn_less_recv_cb[index].last_recv_len);
}

static void reset_connless_recv_cb(int index)
{
	if (index > TEST_CONNLESS_CB_MAX) {
		zassert_true(false, "Invalid callback index %d", index);
		return;
	}

	k_sem_reset(&test_conn_less_recv_cb[index].sem);
	atomic_clear(test_conn_less_recv_cb[index].psm);
	atomic_clear(test_conn_less_recv_cb[index].last_recv_len);
}

static void set_connless_recv_cb(int index, uint16_t psm, struct net_buf *buf)
{
	if (index > TEST_CONNLESS_CB_MAX) {
		return;
	}

	atomic_set(test_conn_less_recv_cb[index].psm, psm);
	atomic_set(test_conn_less_recv_cb[index].last_recv_len, buf->len);
	k_sem_give(&test_conn_less_recv_cb[index].sem);
}

static void wait_connless_recv_cb(int index, uint16_t psm, uint16_t len)
{
	int err;

	if (index > TEST_CONNLESS_CB_MAX) {
		zassert_true(false, "Invalid callback index %d", index);
		return;
	}

	err = k_sem_take(&test_conn_less_recv_cb[index].sem, K_SECONDS(10));
	zassert_equal(err, 0, "Timeout waiting for connless RX (err %d)", err);
	if (psm != 0) {
		zassert_equal(atomic_get(test_conn_less_recv_cb[index].psm), psm,
			"Unexpected PSM 0x%04x", psm);
	}
	if (len > 0) {
		zassert_equal(atomic_get(test_conn_less_recv_cb[index].last_recv_len), len,
			"Unexpected length %u", len);
	}
}

static void connless_recv_cb(struct bt_conn *conn, uint16_t psm, struct net_buf *buf)
{
	LOG_DBG("Received connectionless data: PSM 0x%04x len %u", psm, buf->len);
	LOG_HEXDUMP_DBG(buf->data, buf->len, "connless RX data:");

	set_connless_recv_cb(TEST_CONNLESS_CB_IDX, psm, buf);
}

static struct bt_l2cap_br_connless_cb connless_cb = {
	.psm  = TEST_CONNLESS_PSM,
	.recv = connless_recv_cb,
};

static void connless_recv_other_cb(struct bt_conn *conn, uint16_t psm, struct net_buf *buf)
{
	LOG_DBG("Received connectionless data (other): PSM 0x%04x len %u", psm, buf->len);
	LOG_HEXDUMP_DBG(buf->data, buf->len, "connless other RX data:");

	set_connless_recv_cb(TEST_CONNLESS_CB_OTHER_IDX, psm, buf);
}

static struct bt_l2cap_br_connless_cb connless_other_cb = {
	.psm  = TEST_CONNLESS_PSM_OTHER,
	.recv = connless_recv_other_cb,
};

static void connless_any_recv_cb(struct bt_conn *conn, uint16_t psm, struct net_buf *buf)
{
	LOG_DBG("Wildcard received: PSM 0x%04x len %u", psm, buf->len);
	LOG_HEXDUMP_DBG(buf->data, buf->len, "connless with any psm RX data:");

	set_connless_recv_cb(TEST_CONNLESS_CB_ANY, psm, buf);
}

static struct bt_l2cap_br_connless_cb connless_any_cb = {
	.psm  = 0,
	.recv = connless_any_recv_cb,
};

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
	.connected    = br_connected,
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
		zassert_equal(err, 0, "Failed to find peer device (err %d)", err);
		zassert_true(atomic_test_bit(test_flags, TEST_FLAG_DEVICE_FOUND),
			     "Peer device not found");

		err = bt_br_discovery_stop();
		if ((err != 0) && (err != -EALREADY)) {
			LOG_ERR("Failed to stop GAP discovery (err %d)", err);
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

static void connless_send(uint16_t psm, const void *payload, size_t payload_len)
{
	struct net_buf *buf;
	int err;
	int retries = 3;

	buf = net_buf_alloc(&data_tx_pool, K_FOREVER);
	zassert_not_null(buf, "Unable to allocate Tx buffer");
	net_buf_reserve(buf, BT_L2CAP_CONNLESS_RESERVE);
	net_buf_add_mem(buf, payload, payload_len);

	do {
		err = bt_l2cap_br_connless_send(br_conn, psm, buf);
		if (err == -EOPNOTSUPP) {
			k_sleep(K_SECONDS(1));
		}
		retries--;
	} while (err == -EOPNOTSUPP && retries > 0);
	zassert_equal(err, 0, "bt_l2cap_br_connless_send failed (err %d)", err);
}

ZTEST(l2cap_connless_device1, test_01_basic_send_recv)
{
	static const char tx_data[] = "Hello connless peer";
	int err;

	err = bt_l2cap_br_connless_register(&connless_cb);
	zassert_equal(err, 0, "connless register failed (err %d)", err);

	br_connect_to_peer();
	zassert_true(br_conn != NULL, "Connect handle is NULL");

	reset_connless_recv_cb(TEST_CONNLESS_CB_IDX);

	/* Send connectionless data to peer */
	connless_send(TEST_CONNLESS_PSM, tx_data, sizeof(tx_data));

	/* Wait for the peer to send back connectionless data */
	wait_connless_recv_cb(TEST_CONNLESS_CB_IDX, TEST_CONNLESS_PSM, 0);

	br_disconnect_from_peer();
}

ZTEST(l2cap_connless_device2, test_01_basic_send_recv)
{
	static const char tx_data[] = "Hello connless initiator";
	int err;

	err = bt_l2cap_br_connless_register(&connless_cb);
	zassert_equal(err, 0, "connless register failed (err %d)", err);

	reset_connless_recv_cb(TEST_CONNLESS_CB_IDX);

	test_wait_for_connect();

	/* Wait for connectionless data from peer */
	wait_connless_recv_cb(TEST_CONNLESS_CB_IDX, TEST_CONNLESS_PSM, 0);

	/* Send connectionless data back */
	connless_send(TEST_CONNLESS_PSM, tx_data, sizeof(tx_data));

	test_wait_for_disconnect(K_SECONDS(30));
}

ZTEST(l2cap_connless_device1, test_02_multiple_psm)
{
	static const char tx_psm1[] = "data for psm1";
	static const char tx_psm2[] = "data for psm2";
	int err;

	/* Register PSM1 callback */
	err = bt_l2cap_br_connless_register(&connless_cb);
	zassert_equal(err, 0, "connless register PSM1 failed (err %d)", err);

	/* Register PSM2 callback */
	err = bt_l2cap_br_connless_register(&connless_other_cb);
	zassert_equal(err, 0, "connless register PSM2 failed (err %d)", err);

	/* Register PSM3 callback */
	err = bt_l2cap_br_connless_register(&connless_any_cb);
	zassert_equal(err, 0, "connless register PSM2 failed (err %d)", err);

	br_connect_to_peer();
	zassert_true(br_conn != NULL, "Connect handle is NULL");

	reset_connless_recv_cb(TEST_CONNLESS_CB_IDX);
	reset_connless_recv_cb(TEST_CONNLESS_CB_OTHER_IDX);
	reset_connless_recv_cb(TEST_CONNLESS_CB_ANY);

	/* Send on PSM1 */
	connless_send(TEST_CONNLESS_PSM, tx_psm1, sizeof(tx_psm1));

	/* Wait for receive on PSM1 */
	wait_connless_recv_cb(TEST_CONNLESS_CB_IDX, TEST_CONNLESS_PSM, 0);
	wait_connless_recv_cb(TEST_CONNLESS_CB_ANY, TEST_CONNLESS_PSM, 0);

	/* Send on PSM2 */
	connless_send(TEST_CONNLESS_PSM_OTHER, tx_psm2, sizeof(tx_psm2));

	/* Wait for receive on PSM1 */
	wait_connless_recv_cb(TEST_CONNLESS_CB_OTHER_IDX, TEST_CONNLESS_PSM_OTHER, 0);
	wait_connless_recv_cb(TEST_CONNLESS_CB_ANY, TEST_CONNLESS_PSM_OTHER, 0);

	br_disconnect_from_peer();
}

ZTEST(l2cap_connless_device2, test_02_multiple_psm)
{
	static const char tx_psm1[] = "reply for psm1";
	static const char tx_psm2[] = "reply for psm2";
	int err;

	/* Register PSM1 callback */
	err = bt_l2cap_br_connless_register(&connless_cb);
	zassert_equal(err, 0, "connless register PSM1 failed (err %d)", err);

	/* Register PSM2 callback */
	err = bt_l2cap_br_connless_register(&connless_other_cb);
	zassert_equal(err, 0, "connless register PSM2 failed (err %d)", err);

	/* Register PSM3 callback */
	err = bt_l2cap_br_connless_register(&connless_any_cb);
	zassert_equal(err, 0, "connless register PSM2 failed (err %d)", err);

	reset_connless_recv_cb(TEST_CONNLESS_CB_IDX);
	reset_connless_recv_cb(TEST_CONNLESS_CB_OTHER_IDX);
	reset_connless_recv_cb(TEST_CONNLESS_CB_ANY);

	test_wait_for_connect();

	/* Wait for receive on PSM1 */
	wait_connless_recv_cb(TEST_CONNLESS_CB_IDX, TEST_CONNLESS_PSM, 0);
	wait_connless_recv_cb(TEST_CONNLESS_CB_ANY, TEST_CONNLESS_PSM, 0);

	/* Send on PSM1 */
	connless_send(TEST_CONNLESS_PSM, tx_psm1, sizeof(tx_psm1));

	/* Wait for receive on PSM2 */
	wait_connless_recv_cb(TEST_CONNLESS_CB_OTHER_IDX, TEST_CONNLESS_PSM_OTHER, 0);
	wait_connless_recv_cb(TEST_CONNLESS_CB_ANY, TEST_CONNLESS_PSM_OTHER, 0);

	/* Send replies on both PSMs */
	connless_send(TEST_CONNLESS_PSM_OTHER, tx_psm2, sizeof(tx_psm2));

	test_wait_for_disconnect(K_SECONDS(30));
}

ZTEST(l2cap_connless_device1, test_03_register_unregister_errors)
{
	static const char tx_data[] = "wildcard test data";
	int err;

	/* Register PSM callback */
	err = bt_l2cap_br_connless_register(&connless_cb);
	zassert_equal(err, 0, "connless register failed (err %d)", err);

	/* Registering the same callback again must fail with -EEXIST */
	err = bt_l2cap_br_connless_register(&connless_cb);
	zassert_equal(err, -EEXIST, "Expected -EEXIST, got %d", err);

	/* Register wildcard callback */
	err = bt_l2cap_br_connless_register(&connless_any_cb);
	zassert_equal(err, 0, "wildcard register failed (err %d)", err);

	reset_connless_recv_cb(TEST_CONNLESS_CB_IDX);
	reset_connless_recv_cb(TEST_CONNLESS_CB_OTHER_IDX);
	reset_connless_recv_cb(TEST_CONNLESS_CB_ANY);

	br_connect_to_peer();
	zassert_true(br_conn != NULL, "Connect handle is NULL");

	/* Send data on PSM1 - the wildcard callback should also fire */
	connless_send(TEST_CONNLESS_PSM, tx_data, sizeof(tx_data));

	/* Wait for PSM-specific reception from peer */
	wait_connless_recv_cb(TEST_CONNLESS_CB_IDX, TEST_CONNLESS_PSM, 0);
	wait_connless_recv_cb(TEST_CONNLESS_CB_ANY, TEST_CONNLESS_PSM, 0);

	/* Unregister PSM-specific callback */
	err = bt_l2cap_br_connless_unregister(&connless_cb);
	zassert_equal(err, 0, "unregister failed (err %d)", err);

	/* Second unregister must fail with -ENOENT */
	err = bt_l2cap_br_connless_unregister(&connless_cb);
	zassert_equal(err, -ENOENT, "Expected -ENOENT, got %d", err);

	/* Unregister NULL must fail with -EINVAL */
	err = bt_l2cap_br_connless_unregister(NULL);
	zassert_equal(err, -EINVAL, "Expected -EINVAL for NULL cb, got %d", err);

	/* Unregister wildcard */
	err = bt_l2cap_br_connless_unregister(&connless_any_cb);
	zassert_equal(err, 0, "wildcard unregister failed (err %d)", err);

	br_disconnect_from_peer();
}

ZTEST(l2cap_connless_device2, test_03_register_unregister_errors)
{
	static const char tx_data[] = "reply to wildcard test";
	int err;

	err = bt_l2cap_br_connless_register(&connless_cb);
	zassert_equal(err, 0, "connless register failed (err %d)", err);

	reset_connless_recv_cb(TEST_CONNLESS_CB_IDX);
	reset_connless_recv_cb(TEST_CONNLESS_CB_OTHER_IDX);
	reset_connless_recv_cb(TEST_CONNLESS_CB_ANY);

	test_wait_for_connect();

	/* Wait for data from peer */
	wait_connless_recv_cb(TEST_CONNLESS_CB_IDX, TEST_CONNLESS_PSM, 0);

	/* Send reply */
	connless_send(TEST_CONNLESS_PSM, tx_data, sizeof(tx_data));

	test_wait_for_disconnect(K_SECONDS(30));
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

	LOG_DBG("  Target %s found", br_addr);
	atomic_set_bit(test_flags, TEST_FLAG_DEVICE_FOUND);
	k_sem_give(&br_discovery_found_sem);
}

static struct bt_br_discovery_cb br_discover = {
	.recv    = br_discover_recv,
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
	err = bt_disable();
	zassert_equal(err, 0, "Bluetooth de-init failed (err %d)", err);
	LOG_DBG("Bluetooth de-initialized");
}

static void before(void *f)
{
	atomic_clear(test_flags);

	k_sem_reset(&br_connect_changed_sem);
	k_sem_reset(&br_discovery_found_sem);

	init_connless_recv_cb(TEST_CONNLESS_CB_IDX);
	init_connless_recv_cb(TEST_CONNLESS_CB_OTHER_IDX);
	init_connless_recv_cb(TEST_CONNLESS_CB_ANY);
}

static void after(void *f)
{
	/* Unregister any leftover callbacks to avoid -EEXIST in next test */
	(void)bt_l2cap_br_connless_unregister(&connless_cb);
	(void)bt_l2cap_br_connless_unregister(&connless_other_cb);
	(void)bt_l2cap_br_connless_unregister(&connless_any_cb);

	br_disconnect_from_peer();
}

ZTEST_SUITE(l2cap_connless_device1, NULL, setup, before, after, teardown);
ZTEST_SUITE(l2cap_connless_device2, NULL, setup, before, after, teardown);
