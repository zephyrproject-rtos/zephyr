/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

#define LOG_MODULE_NAME test_classic_l2cap_sim_server
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_TEST_CLASSIC_L2CAP_SIM_LOG_LEVEL);

static K_SEM_DEFINE(br_connect_changed_sem, 0, 1);

static ATOMIC_DEFINE(test_flags, 32);

#define TEST_FLAG_CONN_CONNECTED  0

static struct bt_conn *br_conn;

static void br_connected(struct bt_conn *conn, uint8_t conn_err)
{
	LOG_DBG("connected: conn %p err 0x%02x", (void *)conn, conn_err);
	if (conn_err == 0 && br_conn == NULL) {
		br_conn = conn;
		k_sem_give(&br_connect_changed_sem);
		atomic_set_bit(test_flags, TEST_FLAG_CONN_CONNECTED);
	}
}

static void br_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_DBG("disconnected: conn %p reason 0x%02x", (void *)conn, reason);
	br_conn = NULL;
	k_sem_give(&br_connect_changed_sem);
	atomic_clear_bit(test_flags, TEST_FLAG_CONN_CONNECTED);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = br_connected,
	.disconnected = br_disconnected,
};

enum {
	TEST_L2CAP_FLAG_CONNECTED = 0,
	TEST_L2CAP_FLAG_RX_DISABLED = 1,

	TEST_L2CAP_NUM_FLAGS,
};

NET_BUF_POOL_FIXED_DEFINE(data_tx_pool, CONFIG_TEST_L2CAP_SERVER_TX_DATA_POOL_COUNT,
			  BT_L2CAP_SDU_BUF_SIZE(CONFIG_TEST_L2CAP_SERVER_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
NET_BUF_POOL_FIXED_DEFINE(data_rx_pool, CONFIG_TEST_L2CAP_SERVER_RX_DATA_POOL_COUNT,
			  CONFIG_TEST_L2CAP_SERVER_MTU,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

struct bt_l2cap_br_server {
	struct bt_l2cap_server server;
#if defined(CONFIG_BT_L2CAP_RET_FC)
	uint8_t mode;
	bool optional;
	bool extended_control;
	bool no_fcs;
#endif /* CONFIG_BT_L2CAP_RET_FC */
};

struct l2cap_br_chan {
	struct bt_l2cap_br_chan chan;
	struct k_sem connect_changed_sem;
	struct k_sem rx_sem;
	struct k_sem tx_sem;
	ATOMIC_DEFINE(flags, TEST_L2CAP_NUM_FLAGS);
#if defined(CONFIG_BT_L2CAP_RET_FC)
	struct k_fifo rx_fifo;
#endif /* CONFIG_BT_L2CAP_RET_FC */
};

static int l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct l2cap_br_chan *br_chan = CONTAINER_OF(chan, struct l2cap_br_chan, chan.chan);
	int err = 0;

	LOG_DBG("Incoming data channel %p len %u", chan, buf->len);
	LOG_HEXDUMP_DBG(buf->data, buf->len, "RX data:");

	if (atomic_test_and_clear_bit(br_chan->flags, TEST_L2CAP_FLAG_RX_DISABLED)) {
		LOG_DBG("RX disabled, discarding data");
		return -EINVAL;
	}

#if defined(CONFIG_BT_L2CAP_RET_FC)
	if (br_chan->chan.rx.mode != BT_L2CAP_BR_LINK_MODE_BASIC) {
		k_fifo_put(&br_chan->rx_fifo, buf);
		err = -EINPROGRESS;
	}
#endif /* CONFIG_BT_L2CAP_RET_FC */
	k_sem_give(&br_chan->rx_sem);
	return err;

}

static void l2cap_connected(struct bt_l2cap_chan *chan)
{
	struct l2cap_br_chan *br_chan = CONTAINER_OF(chan, struct l2cap_br_chan, chan.chan);

	LOG_DBG("Channel %p connected", chan);

	atomic_set_bit(br_chan->flags, TEST_L2CAP_FLAG_CONNECTED);

#if defined(CONFIG_BT_L2CAP_RET_FC)
	switch (br_chan->chan.rx.mode) {
	case BT_L2CAP_BR_LINK_MODE_BASIC:
		LOG_DBG("It is basic mode");
		break;
	case BT_L2CAP_BR_LINK_MODE_RET:
		LOG_DBG("It is retransmission mode");
		break;
	case BT_L2CAP_BR_LINK_MODE_FC:
		LOG_DBG("It is flow control mode");
		break;
	case BT_L2CAP_BR_LINK_MODE_ERET:
		LOG_DBG("It is enhance retransmission mode");
		break;
	case BT_L2CAP_BR_LINK_MODE_STREAM:
		LOG_DBG("It is streaming mode");
		break;
	default:
		atomic_clear_bit(br_chan->flags, TEST_L2CAP_FLAG_CONNECTED);
		LOG_ERR("It is unknown mode");
		break;
	}
#endif /* CONFIG_BT_L2CAP_RET_FC */
	k_sem_give(&br_chan->connect_changed_sem);
}

static void l2cap_disconnected(struct bt_l2cap_chan *chan)
{
#if defined(CONFIG_BT_L2CAP_RET_FC)
	struct net_buf *buf;
#endif /* CONFIG_BT_L2CAP_RET_FC */
	struct l2cap_br_chan *br_chan = CONTAINER_OF(chan, struct l2cap_br_chan, chan.chan);

	LOG_DBG("Channel %p disconnected", chan);

#if defined(CONFIG_BT_L2CAP_RET_FC)
	do {
		buf = k_fifo_get(&br_chan->rx_fifo, K_NO_WAIT);
		if (buf != NULL) {
			net_buf_unref(buf);
		}
	} while (buf != NULL);
#endif /* CONFIG_BT_L2CAP_RET_FC */

	atomic_clear_bit(br_chan->flags, TEST_L2CAP_FLAG_CONNECTED);
	k_sem_give(&br_chan->connect_changed_sem);
}

static struct net_buf *l2cap_alloc_buf(struct bt_l2cap_chan *chan)
{
	struct net_buf *buf;

	LOG_DBG("Channel %p requires buffer", chan);

	buf = net_buf_alloc(&data_rx_pool, K_NO_WAIT);
	if (buf == NULL) {
		LOG_WRN("Unable to allocate buffer for L2CAP channel");
	}
	return buf;
}

#if defined(CONFIG_BT_L2CAP_SEG_RECV)
static void seg_recv(struct bt_l2cap_chan *chan, size_t sdu_len, off_t seg_offset,
		     struct net_buf_simple *seg)
{
	LOG_DBG("Incoming data channel %p SDU len %u offset %lu len %u", chan, sdu_len,
		seg_offset, seg->len);
}
#endif /* CONFIG_BT_L2CAP_SEG_RECV */

static const struct bt_l2cap_chan_ops l2cap_ops = {
	.alloc_buf = l2cap_alloc_buf,
	.recv = l2cap_recv,
	.connected = l2cap_connected,
	.disconnected = l2cap_disconnected,
#if defined(CONFIG_BT_L2CAP_SEG_RECV)
	.seg_recv = seg_recv,
#endif /* CONFIG_BT_L2CAP_SEG_RECV */
};

static struct l2cap_br_chan l2cap_chan = {
	.chan = {
		.chan.ops = &l2cap_ops,
		/* Set for now min. MTU */
		.rx.mtu = CONFIG_TEST_L2CAP_SERVER_MTU,
	},
	.connect_changed_sem = Z_SEM_INITIALIZER(l2cap_chan.connect_changed_sem, 0, 1),
	.rx_sem = Z_SEM_INITIALIZER(l2cap_chan.rx_sem, 0,
			CONFIG_TEST_L2CAP_SERVER_RX_DATA_POOL_COUNT),
	.tx_sem = Z_SEM_INITIALIZER(l2cap_chan.tx_sem, 0,
			CONFIG_TEST_L2CAP_SERVER_TX_DATA_POOL_COUNT),
#if defined(CONFIG_BT_L2CAP_RET_FC)
	.rx_fifo = Z_FIFO_INITIALIZER(l2cap_chan.rx_fifo),
#endif /* CONFIG_BT_L2CAP_RET_FC */
};

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			struct bt_l2cap_chan **chan)
{
	__maybe_unused struct bt_l2cap_br_server *br_server;

	br_server = CONTAINER_OF(server, struct bt_l2cap_br_server, server);

	LOG_DBG("Incoming BR/EDR conn %p", conn);

	if (l2cap_chan.chan.chan.conn != NULL) {
		LOG_ERR("No channels available");
		return -ENOMEM;
	}

	*chan = &l2cap_chan.chan.chan;

	memset(&l2cap_chan.chan.rx, 0, sizeof(l2cap_chan.chan.rx));
	memset(&l2cap_chan.chan.tx, 0, sizeof(l2cap_chan.chan.tx));
	l2cap_chan.chan.rx.mtu = CONFIG_TEST_L2CAP_SERVER_MTU;
#if defined(CONFIG_BT_L2CAP_RET_FC)
	l2cap_chan.chan.rx.extended_control = br_server->extended_control;
	l2cap_chan.chan.rx.optional = br_server->optional;
	if (br_server->no_fcs) {
		l2cap_chan.chan.rx.fcs = BT_L2CAP_BR_FCS_NO;
	} else {
		l2cap_chan.chan.rx.fcs = BT_L2CAP_BR_FCS_16BIT;
	}

	l2cap_chan.chan.rx.mode = br_server->mode;
	l2cap_chan.chan.rx.max_window = CONFIG_BT_L2CAP_MAX_WINDOW_SIZE;

	switch (l2cap_chan.chan.rx.mode) {
	case BT_L2CAP_BR_LINK_MODE_STREAM:
		l2cap_chan.chan.rx.max_transmit = 0;
		break;
	case BT_L2CAP_BR_LINK_MODE_ERET:
		l2cap_chan.chan.rx.max_transmit = 3;
		break;
	case BT_L2CAP_BR_LINK_MODE_FC:
		l2cap_chan.chan.rx.max_transmit = 3;
		break;
	case BT_L2CAP_BR_LINK_MODE_RET:
		l2cap_chan.chan.rx.max_transmit = 3;
		break;
	case BT_L2CAP_BR_LINK_MODE_BASIC:
		l2cap_chan.chan.rx.max_transmit = 0;
		break;
	default:
		LOG_WRN("Unknown mode %u", l2cap_chan.chan.rx.mode);
		return -EINVAL;
	}
#endif /* CONFIG_BT_L2CAP_RET_FC */
	return 0;
}

static struct bt_l2cap_br_server l2cap_server = {
	.server = {
		.accept = l2cap_accept,
	},
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

static void test_wait_for_l2cap_connect(uint8_t mode)
{
	int err;

	err = k_sem_take(&l2cap_chan.connect_changed_sem, K_SECONDS(30));
	zassert_equal(err, 0, "L2CAP connection timeout (err %d)", err);
	zassert_true(atomic_test_bit(l2cap_chan.flags, TEST_L2CAP_FLAG_CONNECTED),
		     "L2CAP connection failed");

#if defined(CONFIG_BT_L2CAP_RET_FC)
	zassert_equal(mode, l2cap_chan.chan.rx.mode, "Mode mismatch (expected %u got %u)", mode,
		      l2cap_chan.chan.rx.mode);
#endif /* CONFIG_BT_L2CAP_RET_FC */
}

static void test_recv_complete(uint16_t len)
{
#if defined(CONFIG_BT_L2CAP_RET_FC)
	int err;
	struct net_buf *buf;

	len = MIN(l2cap_chan.chan.rx.mtu, len);

	do {
		buf = k_fifo_get(&l2cap_chan.rx_fifo, K_NO_WAIT);
		if (buf != NULL) {
			zassert_true(buf->len >= len, "RX len mismatched %u != %u", buf->len, len);
			err = bt_l2cap_chan_recv_complete(&l2cap_chan.chan.chan, buf);
			if (err != 0) {
				LOG_ERR("Failed to complete receive (err %d)", err);
			}
		}
	} while (buf != NULL);
#endif /* CONFIG_BT_L2CAP_RET_FC */
}

static void test_wait_for_l2cap_recv(bool release_buffer, uint16_t len)
{
	int err;

	err = k_sem_take(&l2cap_chan.rx_sem, K_SECONDS(30));
	zassert_equal(err, 0, "Receive timeout (err %d)", err);

	if (!release_buffer) {
		return;
	}

	test_recv_complete(len);
}

static void test_wait_for_l2cap_disconnect(void)
{
	int err;

	err = k_sem_take(&l2cap_chan.connect_changed_sem, K_SECONDS(30));
	zassert_equal(err, 0, "L2CAP disconnection timeout (err %d)", err);
	zassert_false(atomic_test_bit(l2cap_chan.flags, TEST_L2CAP_FLAG_CONNECTED),
		     "L2CAP disconnection failed");
}

#define TEST_SERVICE_UUID_32_VALUE 0x11223344
#define TEST_SERVICE_UUID \
	BT_SDP_ARRAY_UUID_128(TEST_SERVICE_UUID_32_VALUE, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb)

static struct bt_sdp_attribute test_server1_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 17),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID128),
			TEST_SERVICE_UUID,
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				&l2cap_server.server.psm,
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("test server"),
};

static struct bt_sdp_record test_server1_rec = BT_SDP_RECORD(test_server1_attrs);

static void test_l2cap_send_str(const char *str)
{
	struct net_buf *buf;
	int err;

	LOG_DBG("Send string: %s", str);
	k_sem_reset(&l2cap_chan.tx_sem);
	buf = net_buf_alloc(&data_tx_pool, K_NO_WAIT);
	zassert_not_null(buf, "Unable to allocate Tx buffer");
	net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, str, strlen(str));
	err = bt_l2cap_chan_send(&l2cap_chan.chan.chan, buf);
	zassert_equal(err, 0, "L2CAP channel send failed (err %d)", err);
}

static void test_l2cap_send_buf(uint16_t len)
{
	struct net_buf *buf;
	uint8_t *data;
	int err;

	len = MIN(l2cap_chan.chan.tx.mtu, len);

	LOG_DBG("Send buffer: %u bytes", len);
	k_sem_reset(&l2cap_chan.tx_sem);
	buf = net_buf_alloc(&data_tx_pool, K_NO_WAIT);
	zassert_not_null(buf, "Unable to allocate Tx buffer");
	net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);
	data = net_buf_add(buf, len);
	memset(data, 0xAA, len);
	err = bt_l2cap_chan_send(&l2cap_chan.chan.chan, buf);
	zassert_equal(err, 0, "L2CAP channel send failed (err %d)", err);
}

static void test_l2cap_conn_disconn_sequence(uint8_t target_mode, const char *send_str,
					     uint8_t next_mode, bool next_optional)
{
	test_wait_for_l2cap_connect(target_mode);

	test_wait_for_l2cap_recv(true, 0);

	test_l2cap_send_str(send_str);

#if defined(CONFIG_BT_L2CAP_RET_FC)
	l2cap_server.mode = next_mode;
	l2cap_server.optional = next_optional;
	l2cap_server.no_fcs = false;
	l2cap_server.extended_control = false;
#endif /* CONFIG_BT_L2CAP_RET_FC */

	test_wait_for_l2cap_disconnect();
}

static void test_l2cap_conn_disconn_sequence_with_len(uint8_t target_mode, uint16_t len,
						      uint8_t next_mode, bool next_optional)
{
	test_wait_for_l2cap_connect(target_mode);

	test_wait_for_l2cap_recv(true, len);

	test_l2cap_send_buf(len);

#if defined(CONFIG_BT_L2CAP_RET_FC)
	l2cap_server.mode = next_mode;
	l2cap_server.optional = next_optional;
	l2cap_server.no_fcs = false;
	l2cap_server.extended_control = false;
#endif /* CONFIG_BT_L2CAP_RET_FC */

	test_wait_for_l2cap_disconnect();
}

ZTEST(l2cap_server, test_01_basic_mode)
{
	int err;

	LOG_DBG("Register L2CAP server");
	k_sem_reset(&l2cap_chan.connect_changed_sem);
	k_sem_reset(&l2cap_chan.rx_sem);
	k_sem_reset(&l2cap_chan.tx_sem);
	l2cap_server.server.psm = 0;
#if defined(CONFIG_BT_L2CAP_RET_FC)
	l2cap_server.mode = BT_L2CAP_BR_LINK_MODE_BASIC;
	l2cap_server.optional = false;
	l2cap_server.no_fcs = false;
	l2cap_server.extended_control = false;
#endif /* CONFIG_BT_L2CAP_RET_FC */
	err = bt_l2cap_br_server_register(&l2cap_server.server);
	zassert_equal(err, 0, "Failed to register L2CAP server (err %d)", err);

	test_wait_for_connect();

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_server::test_01_basic_mode",
					 BT_L2CAP_BR_LINK_MODE_BASIC, false);

	test_wait_for_disconnect(K_SECONDS(30));
}

ZTEST(l2cap_server, test_02_basic_mode_optional)
{
	int err;

	LOG_DBG("Register L2CAP server");
	k_sem_reset(&l2cap_chan.connect_changed_sem);
	k_sem_reset(&l2cap_chan.rx_sem);
	k_sem_reset(&l2cap_chan.tx_sem);
	l2cap_server.server.psm = 0;
#if defined(CONFIG_BT_L2CAP_RET_FC)
	l2cap_server.mode = BT_L2CAP_BR_LINK_MODE_BASIC;
	l2cap_server.optional = false;
	l2cap_server.no_fcs = false;
	l2cap_server.extended_control = false;
#endif /* CONFIG_BT_L2CAP_RET_FC */
	err = bt_l2cap_br_server_register(&l2cap_server.server);
	zassert_equal(err, 0, "Failed to register L2CAP server (err %d)", err);

	test_wait_for_connect();

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_server::test_02_basic_mode::basic_mode",
					 BT_L2CAP_BR_LINK_MODE_ERET, false);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_ERET,
					 "l2cap_server::test_02_basic_mode::eret_mode",
					 BT_L2CAP_BR_LINK_MODE_STREAM, false);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_STREAM,
					 "l2cap_server::test_02_basic_mode::stream_mode",
					 BT_L2CAP_BR_LINK_MODE_ERET, true);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_server::test_02_basic_mode::basic_mode",
					 BT_L2CAP_BR_LINK_MODE_STREAM, true);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_ERET,
					 "l2cap_server::test_02_basic_mode::eret_mode",
					 BT_L2CAP_BR_LINK_MODE_ERET, true);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_STREAM,
					 "l2cap_server::test_02_basic_mode::stream_mode",
					 BT_L2CAP_BR_LINK_MODE_BASIC, true);

	test_wait_for_l2cap_disconnect();

	test_wait_for_l2cap_disconnect();

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_server::test_02_basic_mode::basic_mode",
					 BT_L2CAP_BR_LINK_MODE_BASIC, true);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_server::test_02_basic_mode::basic_mode",
					 BT_L2CAP_BR_LINK_MODE_BASIC, true);

	test_wait_for_disconnect(K_SECONDS(30));
}

ZTEST(l2cap_server, test_03_ret_mode)
{
	int err;

	LOG_DBG("Register L2CAP server");
	k_sem_reset(&l2cap_chan.connect_changed_sem);
	k_sem_reset(&l2cap_chan.rx_sem);
	k_sem_reset(&l2cap_chan.tx_sem);
	l2cap_server.server.psm = 0;
#if defined(CONFIG_BT_L2CAP_RET_FC)
	l2cap_server.mode = BT_L2CAP_BR_LINK_MODE_RET;
	l2cap_server.optional = false;
	l2cap_server.no_fcs = false;
	l2cap_server.extended_control = false;
#endif /* CONFIG_BT_L2CAP_RET_FC */
	err = bt_l2cap_br_server_register(&l2cap_server.server);
	zassert_equal(err, 0, "Failed to register L2CAP server (err %d)", err);

	test_wait_for_connect();

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_RET,
					 "l2cap_server::test_03_ret_mode::ret_mode",
					 BT_L2CAP_BR_LINK_MODE_RET, false);

	test_l2cap_conn_disconn_sequence_with_len(BT_L2CAP_BR_LINK_MODE_RET,
						  CONFIG_TEST_L2CAP_CLIENT_MTU,
						  BT_L2CAP_BR_LINK_MODE_RET, false);

	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_RET);
	for (int i = 0; i < CONFIG_TEST_L2CAP_SERVER_RX_DATA_POOL_COUNT; i++) {
		test_wait_for_l2cap_recv(false, 0);
	}
#if defined(CONFIG_BT_L2CAP_RET_FC)
	LOG_DBG("Waiting timeout %u", l2cap_chan.chan.rx.ret_timeout);
	err = k_sem_take(&l2cap_chan.rx_sem, K_MSEC(l2cap_chan.chan.rx.ret_timeout));
	zassert_true(err < 0, "RX should not be received");

	LOG_DBG("Release RX buffers");
	test_recv_complete(CONFIG_TEST_L2CAP_CLIENT_MTU);

	LOG_DBG("Waiting for 6th RX data (timeout %u)", l2cap_chan.chan.rx.monitor_timeout);
	err = k_sem_take(&l2cap_chan.rx_sem, K_MSEC(l2cap_chan.chan.rx.monitor_timeout));
	zassert_equal(err, 0, "Data not received");
#endif /* CONFIG_BT_L2CAP_RET_FC */

	test_recv_complete(CONFIG_TEST_L2CAP_CLIENT_MTU);

	for (int i = 0; i < CONFIG_TEST_L2CAP_SERVER_TX_DATA_POOL_COUNT; i++) {
		test_l2cap_send_buf(CONFIG_TEST_L2CAP_CLIENT_MTU);
	}

	test_wait_for_l2cap_disconnect();

	atomic_set_bit(l2cap_chan.flags, TEST_L2CAP_FLAG_RX_DISABLED);
	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_RET);

	test_l2cap_send_buf(CONFIG_TEST_L2CAP_CLIENT_MTU);
	test_wait_for_l2cap_disconnect();

	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_RET);
	test_wait_for_l2cap_disconnect();

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_RET,
					 "l2cap_server::test_03_ret_mode::ret_mode",
					 BT_L2CAP_BR_LINK_MODE_RET, false);

	test_wait_for_disconnect(K_SECONDS(30));
}

ZTEST(l2cap_server, test_04_ret_mode_optional)
{
	int err;

	LOG_DBG("Register L2CAP server");
	k_sem_reset(&l2cap_chan.connect_changed_sem);
	k_sem_reset(&l2cap_chan.rx_sem);
	k_sem_reset(&l2cap_chan.tx_sem);
	l2cap_server.server.psm = 0;
#if defined(CONFIG_BT_L2CAP_RET_FC)
	l2cap_server.mode = BT_L2CAP_BR_LINK_MODE_RET;
	l2cap_server.optional = false;
	l2cap_server.no_fcs = false;
	l2cap_server.extended_control = false;
#endif /* CONFIG_BT_L2CAP_RET_FC */
	err = bt_l2cap_br_server_register(&l2cap_server.server);
	zassert_equal(err, 0, "Failed to register L2CAP server (err %d)", err);

	test_wait_for_connect();

	test_wait_for_l2cap_disconnect();

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_RET,
					 "l2cap_server::test_04_ret_mode_optional::ret_mode",
					 BT_L2CAP_BR_LINK_MODE_RET, true);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_server::test_04_ret_mode_optional::basic_mode",
					 BT_L2CAP_BR_LINK_MODE_BASIC, false);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_server::test_04_ret_mode_optional::basic_mode",
					 BT_L2CAP_BR_LINK_MODE_RET, true);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_RET,
					 "l2cap_server::test_04_ret_mode_optional::ret_mode",
					 BT_L2CAP_BR_LINK_MODE_RET, false);

	test_wait_for_disconnect(K_SECONDS(30));
}

ZTEST(l2cap_server, test_05_fc_mode)
{
	int err;

	LOG_DBG("Register L2CAP server");
	k_sem_reset(&l2cap_chan.connect_changed_sem);
	k_sem_reset(&l2cap_chan.rx_sem);
	k_sem_reset(&l2cap_chan.tx_sem);
	l2cap_server.server.psm = 0;
#if defined(CONFIG_BT_L2CAP_RET_FC)
	l2cap_server.mode = BT_L2CAP_BR_LINK_MODE_FC;
	l2cap_server.optional = false;
	l2cap_server.no_fcs = false;
	l2cap_server.extended_control = false;
#endif /* CONFIG_BT_L2CAP_RET_FC */
	err = bt_l2cap_br_server_register(&l2cap_server.server);
	zassert_equal(err, 0, "Failed to register L2CAP server (err %d)", err);

	test_wait_for_connect();

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_FC,
					 "l2cap_server::test_05_fc_mode::fc_mode",
					 BT_L2CAP_BR_LINK_MODE_FC, false);

	test_l2cap_conn_disconn_sequence_with_len(BT_L2CAP_BR_LINK_MODE_FC,
						  CONFIG_TEST_L2CAP_CLIENT_MTU,
						  BT_L2CAP_BR_LINK_MODE_FC, false);

	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_FC);
	for (int i = 0; i < CONFIG_TEST_L2CAP_SERVER_RX_DATA_POOL_COUNT; i++) {
		test_wait_for_l2cap_recv(false, 0);
	}

	for (int i = 0; i < CONFIG_TEST_L2CAP_SERVER_TX_DATA_POOL_COUNT; i++) {
		test_l2cap_send_buf(CONFIG_TEST_L2CAP_CLIENT_MTU);
	}
#if defined(CONFIG_BT_L2CAP_RET_FC)
	LOG_DBG("Waiting timeout %u", l2cap_chan.chan.rx.ret_timeout);
	err = k_sem_take(&l2cap_chan.rx_sem, K_MSEC(l2cap_chan.chan.rx.ret_timeout));
	zassert_true(err < 0, "RX should not be received");

	LOG_DBG("Release RX buffers");
	test_recv_complete(CONFIG_TEST_L2CAP_CLIENT_MTU);

	LOG_DBG("Waiting for 6th RX data (timeout %u)", l2cap_chan.chan.rx.monitor_timeout);
	err = k_sem_take(&l2cap_chan.rx_sem, K_MSEC(l2cap_chan.chan.rx.monitor_timeout));
	zassert_true(err < 0, "RX should not be received");
#endif /* CONFIG_BT_L2CAP_RET_FC */
	test_recv_complete(CONFIG_TEST_L2CAP_CLIENT_MTU);
	test_wait_for_l2cap_disconnect();

	atomic_set_bit(l2cap_chan.flags, TEST_L2CAP_FLAG_RX_DISABLED);
	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_FC);

	test_l2cap_send_buf(CONFIG_TEST_L2CAP_CLIENT_MTU);
	test_wait_for_l2cap_disconnect();

	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_FC);
	test_wait_for_l2cap_disconnect();

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_FC,
					 "l2cap_server::test_05_fc_mode::fc_mode",
					 BT_L2CAP_BR_LINK_MODE_FC, false);

	test_wait_for_disconnect(K_SECONDS(30));
}

ZTEST(l2cap_server, test_06_fc_mode_optional)
{
	int err;

	LOG_DBG("Register L2CAP server");
	k_sem_reset(&l2cap_chan.connect_changed_sem);
	k_sem_reset(&l2cap_chan.rx_sem);
	k_sem_reset(&l2cap_chan.tx_sem);
	l2cap_server.server.psm = 0;
#if defined(CONFIG_BT_L2CAP_RET_FC)
	l2cap_server.mode = BT_L2CAP_BR_LINK_MODE_FC;
	l2cap_server.optional = false;
	l2cap_server.no_fcs = false;
	l2cap_server.extended_control = false;
#endif /* CONFIG_BT_L2CAP_RET_FC */
	err = bt_l2cap_br_server_register(&l2cap_server.server);
	zassert_equal(err, 0, "Failed to register L2CAP server (err %d)", err);

	test_wait_for_connect();

	test_wait_for_l2cap_disconnect();

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_FC,
					 "l2cap_server::test_06_fc_mode_optional::fc_mode",
					 BT_L2CAP_BR_LINK_MODE_FC, true);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_server::test_06_fc_mode_optional::basic_mode",
					 BT_L2CAP_BR_LINK_MODE_BASIC, false);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_server::test_06_fc_mode_optional::basic_mode",
					 BT_L2CAP_BR_LINK_MODE_FC, true);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_FC,
					 "l2cap_server::test_06_fc_mode_optional::fc_mode",
					 BT_L2CAP_BR_LINK_MODE_FC, false);

	test_wait_for_disconnect(K_SECONDS(30));
}

ZTEST(l2cap_server, test_07_eret_mode)
{
	int err;

	LOG_DBG("Register L2CAP server");
	k_sem_reset(&l2cap_chan.connect_changed_sem);
	k_sem_reset(&l2cap_chan.rx_sem);
	k_sem_reset(&l2cap_chan.tx_sem);
	l2cap_server.server.psm = 0;
#if defined(CONFIG_BT_L2CAP_RET_FC)
	l2cap_server.mode = BT_L2CAP_BR_LINK_MODE_ERET;
	l2cap_server.optional = false;
	l2cap_server.no_fcs = false;
	l2cap_server.extended_control = false;
#endif /* CONFIG_BT_L2CAP_RET_FC */
	err = bt_l2cap_br_server_register(&l2cap_server.server);
	zassert_equal(err, 0, "Failed to register L2CAP server (err %d)", err);

	test_wait_for_connect();

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_ERET,
					 "l2cap_server::test_07_eret_mode::eret_mode",
					 BT_L2CAP_BR_LINK_MODE_ERET, false);

	test_l2cap_conn_disconn_sequence_with_len(BT_L2CAP_BR_LINK_MODE_ERET,
						  CONFIG_TEST_L2CAP_CLIENT_MTU,
						  BT_L2CAP_BR_LINK_MODE_ERET, false);

	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_ERET);
	for (int i = 0; i < CONFIG_TEST_L2CAP_SERVER_RX_DATA_POOL_COUNT; i++) {
		test_wait_for_l2cap_recv(false, 0);
	}
#if defined(CONFIG_BT_L2CAP_RET_FC)
	LOG_DBG("Waiting timeout %u", l2cap_chan.chan.rx.ret_timeout);
	err = k_sem_take(&l2cap_chan.rx_sem, K_MSEC(l2cap_chan.chan.rx.ret_timeout));
	zassert_true(err < 0, "RX should not be received");

	LOG_DBG("Release RX buffers");
	test_recv_complete(CONFIG_TEST_L2CAP_CLIENT_MTU);

	LOG_DBG("Waiting for 6th RX data (timeout %u)", l2cap_chan.chan.rx.monitor_timeout);
	err = k_sem_take(&l2cap_chan.rx_sem, K_MSEC(l2cap_chan.chan.rx.monitor_timeout));
	zassert_equal(err, 0, "Data not received");
#endif /* CONFIG_BT_L2CAP_RET_FC */

	test_recv_complete(CONFIG_TEST_L2CAP_CLIENT_MTU);

	for (int i = 0; i < CONFIG_TEST_L2CAP_SERVER_TX_DATA_POOL_COUNT; i++) {
		test_l2cap_send_buf(CONFIG_TEST_L2CAP_CLIENT_MTU);
	}

	test_wait_for_l2cap_disconnect();

	atomic_set_bit(l2cap_chan.flags, TEST_L2CAP_FLAG_RX_DISABLED);
	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_ERET);

	test_l2cap_send_buf(CONFIG_TEST_L2CAP_CLIENT_MTU);
	test_wait_for_l2cap_disconnect();

	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_ERET);
	test_wait_for_l2cap_disconnect();

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_ERET,
					 "l2cap_server::test_07_eret_mode::eret_mode",
					 BT_L2CAP_BR_LINK_MODE_ERET, false);

	test_wait_for_disconnect(K_SECONDS(30));
}

ZTEST(l2cap_server, test_08_eret_mode_optional)
{
	int err;

	LOG_DBG("Register L2CAP server");
	k_sem_reset(&l2cap_chan.connect_changed_sem);
	k_sem_reset(&l2cap_chan.rx_sem);
	k_sem_reset(&l2cap_chan.tx_sem);
	l2cap_server.server.psm = 0;
#if defined(CONFIG_BT_L2CAP_RET_FC)
	l2cap_server.mode = BT_L2CAP_BR_LINK_MODE_ERET;
	l2cap_server.optional = false;
	l2cap_server.no_fcs = false;
	l2cap_server.extended_control = false;
#endif /* CONFIG_BT_L2CAP_RET_FC */
	err = bt_l2cap_br_server_register(&l2cap_server.server);
	zassert_equal(err, 0, "Failed to register L2CAP server (err %d)", err);

	test_wait_for_connect();

	test_wait_for_l2cap_disconnect();

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_ERET,
					 "l2cap_server::test_08_eret_mode_optional::eret_mode",
					 BT_L2CAP_BR_LINK_MODE_ERET, true);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_server::test_08_eret_mode_optional::basic_mode",
					 BT_L2CAP_BR_LINK_MODE_BASIC, false);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_server::test_08_eret_mode_optional::basic_mode",
					 BT_L2CAP_BR_LINK_MODE_ERET, true);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_ERET,
					 "l2cap_server::test_08_eret_mode_optional::eret_mode",
					 BT_L2CAP_BR_LINK_MODE_ERET, false);

	test_wait_for_disconnect(K_SECONDS(30));
}

ZTEST(l2cap_server, test_09_stream_mode)
{
	int err;

	LOG_DBG("Register L2CAP server");
	k_sem_reset(&l2cap_chan.connect_changed_sem);
	k_sem_reset(&l2cap_chan.rx_sem);
	k_sem_reset(&l2cap_chan.tx_sem);
	l2cap_server.server.psm = 0;
#if defined(CONFIG_BT_L2CAP_RET_FC)
	l2cap_server.mode = BT_L2CAP_BR_LINK_MODE_STREAM;
	l2cap_server.optional = false;
	l2cap_server.no_fcs = false;
	l2cap_server.extended_control = false;
#endif /* CONFIG_BT_L2CAP_RET_FC */
	err = bt_l2cap_br_server_register(&l2cap_server.server);
	zassert_equal(err, 0, "Failed to register L2CAP server (err %d)", err);

	test_wait_for_connect();

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_STREAM,
					 "l2cap_server::test_09_stream_mode::stream_mode",
					 BT_L2CAP_BR_LINK_MODE_STREAM, false);

	test_l2cap_conn_disconn_sequence_with_len(BT_L2CAP_BR_LINK_MODE_STREAM,
						  CONFIG_TEST_L2CAP_CLIENT_MTU,
						  BT_L2CAP_BR_LINK_MODE_STREAM, false);

	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_STREAM);
	for (int i = 0; i < CONFIG_TEST_L2CAP_SERVER_RX_DATA_POOL_COUNT; i++) {
		test_wait_for_l2cap_recv(false, 0);
	}

	for (int i = 0; i < CONFIG_TEST_L2CAP_SERVER_TX_DATA_POOL_COUNT; i++) {
		test_l2cap_send_buf(CONFIG_TEST_L2CAP_CLIENT_MTU);
	}
#if defined(CONFIG_BT_L2CAP_RET_FC)
	LOG_DBG("Waiting timeout %u", l2cap_chan.chan.rx.ret_timeout);
	err = k_sem_take(&l2cap_chan.rx_sem, K_MSEC(l2cap_chan.chan.rx.ret_timeout));
	zassert_true(err < 0, "RX should not be received");

	LOG_DBG("Release RX buffers");
	test_recv_complete(CONFIG_TEST_L2CAP_CLIENT_MTU);

	LOG_DBG("Waiting for 6th RX data (timeout %u)", l2cap_chan.chan.rx.monitor_timeout);
	err = k_sem_take(&l2cap_chan.rx_sem, K_MSEC(l2cap_chan.chan.rx.monitor_timeout));
	zassert_true(err < 0, "RX should not be received");
#endif /* CONFIG_BT_L2CAP_RET_FC */
	test_recv_complete(CONFIG_TEST_L2CAP_CLIENT_MTU);
	test_wait_for_l2cap_disconnect();

	atomic_set_bit(l2cap_chan.flags, TEST_L2CAP_FLAG_RX_DISABLED);
	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_STREAM);

	test_l2cap_send_buf(CONFIG_TEST_L2CAP_CLIENT_MTU);
	test_wait_for_l2cap_disconnect();

	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_STREAM);
	test_wait_for_l2cap_disconnect();

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_STREAM,
					 "l2cap_server::test_09_stream_mode::stream_mode",
					 BT_L2CAP_BR_LINK_MODE_STREAM, false);

	test_wait_for_disconnect(K_SECONDS(30));
}

ZTEST(l2cap_server, test_10_stream_mode_optional)
{
	int err;

	LOG_DBG("Register L2CAP server");
	k_sem_reset(&l2cap_chan.connect_changed_sem);
	k_sem_reset(&l2cap_chan.rx_sem);
	k_sem_reset(&l2cap_chan.tx_sem);
	l2cap_server.server.psm = 0;
#if defined(CONFIG_BT_L2CAP_RET_FC)
	l2cap_server.mode = BT_L2CAP_BR_LINK_MODE_STREAM;
	l2cap_server.optional = false;
	l2cap_server.no_fcs = false;
	l2cap_server.extended_control = false;
#endif /* CONFIG_BT_L2CAP_RET_FC */
	err = bt_l2cap_br_server_register(&l2cap_server.server);
	zassert_equal(err, 0, "Failed to register L2CAP server (err %d)", err);

	test_wait_for_connect();

	test_wait_for_l2cap_disconnect();

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_STREAM,
					 "l2cap_server::test_10_stream_mode_optional::stream_mode",
					 BT_L2CAP_BR_LINK_MODE_STREAM, true);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_server::test_10_stream_mode_optional::basic_mode",
					 BT_L2CAP_BR_LINK_MODE_BASIC, false);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_server::test_10_stream_mode_optional::basic_mode",
					 BT_L2CAP_BR_LINK_MODE_STREAM, true);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_STREAM,
					 "l2cap_server::test_10_stream_mode_optional::stream_mode",
					 BT_L2CAP_BR_LINK_MODE_STREAM, false);

	test_wait_for_disconnect(K_SECONDS(30));
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

	test_wait_for_disconnect(K_SECONDS(30));
}

static void *setup(void)
{
	int err;

	LOG_DBG("Initializing Bluetooth");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	zassert_equal(err, 0, "Bluetooth init failed (err %d)", err);

	LOG_DBG("Bluetooth initialized");

	err = bt_br_set_connectable(true, NULL);
	zassert_equal(err, 0, "Failed to set connectable (err %d)", err);

	err = bt_br_set_discoverable(true, true);
	zassert_equal(err, 0, "Failed to set discoverable (err %d)", err);

	err = bt_sdp_register_service(&test_server1_rec);
	zassert_equal(err, 0, "SDP service registration failed (err %d)", err);

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
	k_sem_reset(&br_connect_changed_sem);

	atomic_clear_bit(test_flags, TEST_FLAG_CONN_CONNECTED);
}

static void after(void *f)
{
	br_disconnect_from_peer();

	atomic_clear(l2cap_chan.flags);
}

ZTEST_SUITE(l2cap_server, NULL, setup, before, after, teardown);
