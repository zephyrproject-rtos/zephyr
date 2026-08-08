/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

#define LOG_MODULE_NAME test_classic_l2cap_sim_client
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_TEST_CLASSIC_L2CAP_SIM_LOG_LEVEL);

extern bt_addr_t peer_addr;
static K_SEM_DEFINE(br_connect_changed_sem, 0, 1);

static struct bt_br_discovery_param br_discover_param;
#define BR_DISCOVER_RESULT_COUNT 10
static struct bt_br_discovery_result br_discover_result[BR_DISCOVER_RESULT_COUNT];
static K_SEM_DEFINE(br_discovery_found_sem, 0, 1);

#define TEST_FLAG_DEVICE_FOUND 0
#define TEST_FLAG_PSM_FOUND    1

static ATOMIC_DEFINE(test_flags, 32);

static struct bt_conn *br_conn;
static uint16_t l2cap_psm;

static void br_connected(struct bt_conn *conn, uint8_t conn_err)
{
	LOG_DBG("connected: conn %p err 0x%02x", (void *)conn, conn_err);
	if (conn_err == 0 && br_conn == NULL) {
		br_conn = bt_conn_ref(conn);
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

static K_SEM_DEFINE(br_sdp_discover_sem, 0, 1);
static struct bt_sdp_discover_params sdp_discover_params;
#define SDP_CLIENT_USER_BUF_LEN 4096
NET_BUF_POOL_FIXED_DEFINE(sdp_client_pool, 1, SDP_CLIENT_USER_BUF_LEN,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
static struct bt_uuid_any sdp_uuid;

static void test_sdp_discover_service_search_attr(struct bt_conn *conn, struct bt_uuid *uuid,
						  bt_sdp_discover_func_t func,
						  struct bt_sdp_attribute_id_list *ids)
{
	int err;

	sdp_discover_params.uuid = uuid;
	sdp_discover_params.func = func;
	sdp_discover_params.pool = &sdp_client_pool;
	sdp_discover_params.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;
	sdp_discover_params.ids = ids;

	k_sem_reset(&br_sdp_discover_sem);
	err = bt_sdp_discover(conn, &sdp_discover_params);
	zassert_equal(err, 0, "Failed to start sdp discovery (err %d)", err);

	err = k_sem_take(&br_sdp_discover_sem, K_SECONDS(10));
	zassert_equal(err, 0, "Wait for sdp discover done event timeout (err %d)", err);

	zassert_true(atomic_test_bit(test_flags, TEST_FLAG_PSM_FOUND),
				     "Test case passed bit not set");
}

static int test_get_attr_value_u16(const struct net_buf *buf, uint16_t attr_id,
				   const struct bt_uuid *uuid, uint16_t *value)
{
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value val;
	int err;

	err = bt_sdp_get_attr(buf, attr_id, &attr);
	if (err != 0) {
		LOG_ERR("Record handle not found (err %d)", err);
		return err;
	}

	err = bt_sdp_attr_read(&attr, uuid, &val);
	if (err != 0) {
		LOG_ERR("Failed to read attribute (err %d)", err);
		return err;
	}

	if (val.type != BT_SDP_ATTR_VALUE_TYPE_UINT || val.uint.size != sizeof(*value)) {
		LOG_ERR("Invalid attribute value type or size");
		return -EINVAL;
	}

	*value = val.uint.u16;
	return 0;
}

static uint8_t test_sdp_discover_l2cap_chan_cb(struct bt_conn *conn,
					       struct bt_sdp_client_result *result,
					       const struct bt_sdp_discover_params *params)
{
	int err;
	struct bt_uuid_16 uuid16;

	LOG_DBG("SDP discovery callback");
	if (result == NULL || result->resp_buf == NULL || result->resp_buf->len == 0) {
		LOG_ERR("N/A response received");
		goto failed;
	}

	if (params->type == BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR) {
		uuid16.uuid.type = BT_UUID_TYPE_16;
		uuid16.val = BT_SDP_PROTO_L2CAP;
		err = test_get_attr_value_u16(result->resp_buf, BT_SDP_ATTR_PROTO_DESC_LIST,
					      &uuid16.uuid, &l2cap_psm);
		if (err != 0) {
			goto failed;
		}
	}

	atomic_set_bit(test_flags, TEST_FLAG_PSM_FOUND);
failed:
	k_sem_give(&br_sdp_discover_sem);
	return BT_SDP_DISCOVER_UUID_STOP;
}

enum {
	TEST_L2CAP_FLAG_CONNECTED = 0,
	TEST_L2CAP_FLAG_RX_DISABLED = 1,

	TEST_L2CAP_NUM_FLAGS,
};

NET_BUF_POOL_FIXED_DEFINE(data_tx_pool, CONFIG_TEST_L2CAP_CLIENT_TX_DATA_POOL_COUNT,
			  BT_L2CAP_SDU_BUF_SIZE(CONFIG_TEST_L2CAP_CLIENT_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
NET_BUF_POOL_FIXED_DEFINE(data_rx_pool, CONFIG_TEST_L2CAP_CLIENT_RX_DATA_POOL_COUNT,
			  CONFIG_TEST_L2CAP_CLIENT_MTU,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

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
		.rx.mtu = CONFIG_TEST_L2CAP_CLIENT_MTU,
	},
	.connect_changed_sem = Z_SEM_INITIALIZER(l2cap_chan.connect_changed_sem, 0, 1),
	.rx_sem = Z_SEM_INITIALIZER(l2cap_chan.rx_sem, 0,
			CONFIG_TEST_L2CAP_CLIENT_RX_DATA_POOL_COUNT),
	.tx_sem = Z_SEM_INITIALIZER(l2cap_chan.tx_sem, 0,
			CONFIG_TEST_L2CAP_CLIENT_TX_DATA_POOL_COUNT),
#if defined(CONFIG_BT_L2CAP_RET_FC)
	.rx_fifo = Z_FIFO_INITIALIZER(l2cap_chan.rx_fifo),
#endif /* CONFIG_BT_L2CAP_RET_FC */
};

static void test_wait_for_l2cap_connect(uint8_t mode)
{
	int err;

	LOG_DBG("Waiting for L2CAP connection with mode %u", mode);

	err = k_sem_take(&l2cap_chan.connect_changed_sem, K_SECONDS(30));
	zassert_equal(err, 0, "Connection timeout (err %d)", err);
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

static void test_wait_for_l2cap_disconnect(void)
{
	int err;

	LOG_DBG("Waiting for L2CAP disconnection");

	err = k_sem_take(&l2cap_chan.connect_changed_sem, K_SECONDS(30));
	zassert_equal(err, 0, "disconnection timeout (err %d)", err);
	zassert_false(atomic_test_bit(l2cap_chan.flags, TEST_L2CAP_FLAG_CONNECTED),
		     "L2CAP disconnection failed");
}

static void test_l2cap_conn_create(uint8_t mode, bool optional)
{
	int err;

	k_sem_reset(&l2cap_chan.connect_changed_sem);
	k_sem_reset(&l2cap_chan.rx_sem);
	k_sem_reset(&l2cap_chan.tx_sem);

	memset(&l2cap_chan.chan.rx, 0, sizeof(l2cap_chan.chan.rx));
	memset(&l2cap_chan.chan.tx, 0, sizeof(l2cap_chan.chan.tx));
	l2cap_chan.chan.rx.mtu = CONFIG_TEST_L2CAP_CLIENT_MTU;
#if defined(CONFIG_BT_L2CAP_RET_FC)
	l2cap_chan.chan.rx.mode = mode;
	l2cap_chan.chan.rx.optional = optional;
	l2cap_chan.chan.rx.extended_control = false;
	l2cap_chan.chan.rx.max_window = CONFIG_BT_L2CAP_MAX_WINDOW_SIZE;
	l2cap_chan.chan.rx.max_transmit = 3;
#endif /* CONFIG_BT_L2CAP_RET_FC */
	LOG_DBG("Connecting to L2CAP PSM: %u", l2cap_psm);
	err = bt_l2cap_chan_connect(br_conn, &l2cap_chan.chan.chan, l2cap_psm);
	zassert_equal(err, 0, "L2CAP channel connection failed (err %d)", err);
}

static void test_l2cap_conn_disconn_sequence(uint8_t mode, bool optional, uint8_t target_mode,
					     const char *send_str)
{
	int err;

	test_l2cap_conn_create(mode, optional);

	test_wait_for_l2cap_connect(target_mode);

	test_l2cap_send_str(send_str);

	test_wait_for_l2cap_recv(true, 0);

	err = bt_l2cap_chan_disconnect(&l2cap_chan.chan.chan);
	zassert_equal(err, 0, "L2CAP channel disconnection failed (err %d)", err);

	test_wait_for_l2cap_disconnect();
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
	memset(data, 0xCC, len);
	err = bt_l2cap_chan_send(&l2cap_chan.chan.chan, buf);
	zassert_equal(err, 0, "L2CAP channel send failed (err %d)", err);
}

static void test_l2cap_conn_disconn_sequence_with_len(uint8_t mode, bool optional,
						      uint8_t target_mode, uint16_t len)
{
	int err;

	test_l2cap_conn_create(mode, optional);

	test_wait_for_l2cap_connect(target_mode);

	test_l2cap_send_buf(len);

	test_wait_for_l2cap_recv(true, len);

	err = bt_l2cap_chan_disconnect(&l2cap_chan.chan.chan);
	zassert_equal(err, 0, "L2CAP channel disconnection failed (err %d)", err);

	test_wait_for_l2cap_disconnect();
}

ZTEST(l2cap_client, test_01_basic_mode)
{
	br_connect_to_peer();
	zassert_true(br_conn != NULL, "Connect handle is NULL");

	sdp_uuid.uuid.type = BT_UUID_TYPE_16;
	sdp_uuid.u16.val = BT_SDP_PROTO_L2CAP;
	l2cap_psm = 0xffff;
	test_sdp_discover_service_search_attr(br_conn, &sdp_uuid.uuid,
					      test_sdp_discover_l2cap_chan_cb, NULL);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_BASIC, false,
					 BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_client::test_01_basic_mode");

	br_disconnect_from_peer();
}

static void test_l2cap_conn_failed(uint8_t mode, bool optional, uint8_t target_mode,
				   const char *send_str)
{
	int err;

	k_sem_reset(&l2cap_chan.connect_changed_sem);
	k_sem_reset(&l2cap_chan.rx_sem);
	k_sem_reset(&l2cap_chan.tx_sem);

	memset(&l2cap_chan.chan.rx, 0, sizeof(l2cap_chan.chan.rx));
	memset(&l2cap_chan.chan.tx, 0, sizeof(l2cap_chan.chan.tx));
	l2cap_chan.chan.rx.mtu = CONFIG_TEST_L2CAP_CLIENT_MTU;
#if defined(CONFIG_BT_L2CAP_RET_FC)
	l2cap_chan.chan.rx.mode = mode;
	l2cap_chan.chan.rx.optional = optional;
	l2cap_chan.chan.rx.extended_control = false;
	l2cap_chan.chan.rx.max_window = CONFIG_BT_L2CAP_MAX_WINDOW_SIZE;
#endif /* CONFIG_BT_L2CAP_RET_FC */
	LOG_DBG("Connecting to L2CAP PSM: %u", l2cap_psm);
	err = bt_l2cap_chan_connect(br_conn, &l2cap_chan.chan.chan, l2cap_psm);
	zassert_equal(err, 0, "L2CAP channel connection failed (err %d)", err);

	test_wait_for_l2cap_disconnect();
}

ZTEST(l2cap_client, test_02_basic_mode_optional)
{
	br_connect_to_peer();
	zassert_true(br_conn != NULL, "Connect handle is NULL");

	sdp_uuid.uuid.type = BT_UUID_TYPE_16;
	sdp_uuid.u16.val = BT_SDP_PROTO_L2CAP;
	l2cap_psm = 0xffff;
	test_sdp_discover_service_search_attr(br_conn, &sdp_uuid.uuid,
					      test_sdp_discover_l2cap_chan_cb, NULL);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_ERET, true,
					 BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_client::test_02_basic_mode::basic_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_STREAM, true,
					 BT_L2CAP_BR_LINK_MODE_ERET,
					 "l2cap_client::test_02_basic_mode::eret_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_ERET, true,
					 BT_L2CAP_BR_LINK_MODE_STREAM,
					 "l2cap_client::test_02_basic_mode::stream_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_BASIC, false,
					 BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_client::test_02_basic_mode::basic_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_ERET, false,
					 BT_L2CAP_BR_LINK_MODE_ERET,
					 "l2cap_client::test_02_basic_mode::eret_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_STREAM, false,
					 BT_L2CAP_BR_LINK_MODE_STREAM,
					 "l2cap_client::test_02_basic_mode::stream_mode");

	test_l2cap_conn_failed(BT_L2CAP_BR_LINK_MODE_ERET, false, BT_L2CAP_BR_LINK_MODE_ERET,
			       "l2cap_client::test_02_basic_mode::eret_mode");

	test_l2cap_conn_failed(BT_L2CAP_BR_LINK_MODE_STREAM, false, BT_L2CAP_BR_LINK_MODE_STREAM,
			       "l2cap_client::test_02_basic_mode::eret_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_ERET, true,
					 BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_client::test_02_basic_mode::basic_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_STREAM, true,
					 BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_client::test_02_basic_mode::basic_mode");

	br_disconnect_from_peer();
}

ZTEST(l2cap_client, test_03_ret_mode)
{
	int err;

	br_connect_to_peer();
	zassert_true(br_conn != NULL, "Connect handle is NULL");

	sdp_uuid.uuid.type = BT_UUID_TYPE_16;
	sdp_uuid.u16.val = BT_SDP_PROTO_L2CAP;
	l2cap_psm = 0xffff;
	test_sdp_discover_service_search_attr(br_conn, &sdp_uuid.uuid,
					      test_sdp_discover_l2cap_chan_cb, NULL);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_RET, false,
					 BT_L2CAP_BR_LINK_MODE_RET,
					 "l2cap_client::test_02_basic_mode::basic_mode");

	test_l2cap_conn_disconn_sequence_with_len(BT_L2CAP_BR_LINK_MODE_RET, false,
						  BT_L2CAP_BR_LINK_MODE_RET,
						  CONFIG_TEST_L2CAP_CLIENT_MTU);

	test_l2cap_conn_create(BT_L2CAP_BR_LINK_MODE_RET, false);

	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_RET);
	for (int i = 0; i < CONFIG_TEST_L2CAP_CLIENT_TX_DATA_POOL_COUNT; i++) {
		test_l2cap_send_buf(CONFIG_TEST_L2CAP_SERVER_MTU);
	}

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

	err = bt_l2cap_chan_disconnect(&l2cap_chan.chan.chan);
	zassert_equal(err, 0, "L2CAP channel disconnection failed (err %d)", err);

	test_wait_for_l2cap_disconnect();

	atomic_set_bit(l2cap_chan.flags, TEST_L2CAP_FLAG_RX_DISABLED);
	test_l2cap_conn_create(BT_L2CAP_BR_LINK_MODE_RET, false);
	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_RET);

	test_wait_for_l2cap_disconnect();

	test_l2cap_conn_create(BT_L2CAP_BR_LINK_MODE_RET, false);
	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_RET);

	test_l2cap_send_buf(CONFIG_TEST_L2CAP_SERVER_MTU);

	test_wait_for_l2cap_disconnect();

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_RET, false,
					 BT_L2CAP_BR_LINK_MODE_RET,
					 "l2cap_client::test_02_basic_mode::basic_mode");

	br_disconnect_from_peer();
}

ZTEST(l2cap_client, test_04_ret_mode_optional)
{
	br_connect_to_peer();
	zassert_true(br_conn != NULL, "Connect handle is NULL");

	sdp_uuid.uuid.type = BT_UUID_TYPE_16;
	sdp_uuid.u16.val = BT_SDP_PROTO_L2CAP;
	l2cap_psm = 0xffff;
	test_sdp_discover_service_search_attr(br_conn, &sdp_uuid.uuid,
					      test_sdp_discover_l2cap_chan_cb, NULL);

	test_l2cap_conn_failed(BT_L2CAP_BR_LINK_MODE_BASIC, false, BT_L2CAP_BR_LINK_MODE_BASIC,
			       "l2cap_client::test_04_ret_mode_optional::ret_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_RET, false,
					 BT_L2CAP_BR_LINK_MODE_RET,
					 "l2cap_client::test_04_ret_mode_optional::ret_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_BASIC, false,
					 BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_client::test_04_ret_mode_optional::basic_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_RET, true,
					 BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_client::test_04_ret_mode_optional::basic_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_RET, true,
					 BT_L2CAP_BR_LINK_MODE_RET,
					 "l2cap_client::test_04_ret_mode_optional::ret_mode");

	br_disconnect_from_peer();
}

ZTEST(l2cap_client, test_05_fc_mode)
{
	int err;

	br_connect_to_peer();
	zassert_true(br_conn != NULL, "Connect handle is NULL");

	sdp_uuid.uuid.type = BT_UUID_TYPE_16;
	sdp_uuid.u16.val = BT_SDP_PROTO_L2CAP;
	l2cap_psm = 0xffff;
	test_sdp_discover_service_search_attr(br_conn, &sdp_uuid.uuid,
					      test_sdp_discover_l2cap_chan_cb, NULL);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_FC, false,
					 BT_L2CAP_BR_LINK_MODE_FC,
					 "l2cap_client::test_05_fc_mode::fc_mode");

	test_l2cap_conn_disconn_sequence_with_len(BT_L2CAP_BR_LINK_MODE_FC, false,
						  BT_L2CAP_BR_LINK_MODE_FC,
						  CONFIG_TEST_L2CAP_CLIENT_MTU);

	test_l2cap_conn_create(BT_L2CAP_BR_LINK_MODE_FC, false);

	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_FC);
	for (int i = 0; i < CONFIG_TEST_L2CAP_CLIENT_TX_DATA_POOL_COUNT; i++) {
		test_l2cap_send_buf(CONFIG_TEST_L2CAP_SERVER_MTU);
	}

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
	zassert_true(err < 0, "RX should not be received");
#endif /* CONFIG_BT_L2CAP_RET_FC */
	test_recv_complete(CONFIG_TEST_L2CAP_CLIENT_MTU);

	err = bt_l2cap_chan_disconnect(&l2cap_chan.chan.chan);
	zassert_equal(err, 0, "L2CAP channel disconnection failed (err %d)", err);

	test_wait_for_l2cap_disconnect();

	atomic_set_bit(l2cap_chan.flags, TEST_L2CAP_FLAG_RX_DISABLED);
	test_l2cap_conn_create(BT_L2CAP_BR_LINK_MODE_FC, false);
	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_FC);

	test_wait_for_l2cap_disconnect();

	test_l2cap_conn_create(BT_L2CAP_BR_LINK_MODE_FC, false);
	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_FC);

	test_l2cap_send_buf(CONFIG_TEST_L2CAP_SERVER_MTU);

	test_wait_for_l2cap_disconnect();

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_FC, false,
					 BT_L2CAP_BR_LINK_MODE_FC,
					 "l2cap_client::test_05_fc_mode::fc_mode");

	br_disconnect_from_peer();
}

ZTEST(l2cap_client, test_06_fc_mode_optional)
{
	br_connect_to_peer();
	zassert_true(br_conn != NULL, "Connect handle is NULL");

	sdp_uuid.uuid.type = BT_UUID_TYPE_16;
	sdp_uuid.u16.val = BT_SDP_PROTO_L2CAP;
	l2cap_psm = 0xffff;
	test_sdp_discover_service_search_attr(br_conn, &sdp_uuid.uuid,
					      test_sdp_discover_l2cap_chan_cb, NULL);

	test_l2cap_conn_failed(BT_L2CAP_BR_LINK_MODE_BASIC, false, BT_L2CAP_BR_LINK_MODE_BASIC,
			       "l2cap_client::test_06_fc_mode_optional::fc_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_FC, false,
					 BT_L2CAP_BR_LINK_MODE_FC,
					 "l2cap_client::test_06_fc_mode_optional::fc_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_BASIC, false,
					 BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_client::test_06_fc_mode_optional::basic_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_FC, true,
					 BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_client::test_06_fc_mode_optional::basic_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_FC, true,
					 BT_L2CAP_BR_LINK_MODE_FC,
					 "l2cap_client::test_06_fc_mode_optional::fc_mode");

	br_disconnect_from_peer();
}

ZTEST(l2cap_client, test_07_eret_mode)
{
	int err;

	br_connect_to_peer();
	zassert_true(br_conn != NULL, "Connect handle is NULL");

	sdp_uuid.uuid.type = BT_UUID_TYPE_16;
	sdp_uuid.u16.val = BT_SDP_PROTO_L2CAP;
	l2cap_psm = 0xffff;
	test_sdp_discover_service_search_attr(br_conn, &sdp_uuid.uuid,
					      test_sdp_discover_l2cap_chan_cb, NULL);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_ERET, false,
					 BT_L2CAP_BR_LINK_MODE_ERET,
					 "l2cap_client::test_07_eret_mode::eret_mode");

	test_l2cap_conn_disconn_sequence_with_len(BT_L2CAP_BR_LINK_MODE_ERET, false,
						  BT_L2CAP_BR_LINK_MODE_ERET,
						  CONFIG_TEST_L2CAP_CLIENT_MTU);

	test_l2cap_conn_create(BT_L2CAP_BR_LINK_MODE_ERET, false);

	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_ERET);
	for (int i = 0; i < CONFIG_TEST_L2CAP_CLIENT_TX_DATA_POOL_COUNT; i++) {
		test_l2cap_send_buf(CONFIG_TEST_L2CAP_SERVER_MTU);
	}

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

	err = bt_l2cap_chan_disconnect(&l2cap_chan.chan.chan);
	zassert_equal(err, 0, "L2CAP channel disconnection failed (err %d)", err);

	test_wait_for_l2cap_disconnect();

	atomic_set_bit(l2cap_chan.flags, TEST_L2CAP_FLAG_RX_DISABLED);
	test_l2cap_conn_create(BT_L2CAP_BR_LINK_MODE_ERET, false);
	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_ERET);

	test_wait_for_l2cap_disconnect();

	test_l2cap_conn_create(BT_L2CAP_BR_LINK_MODE_ERET, false);
	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_ERET);

	test_l2cap_send_buf(CONFIG_TEST_L2CAP_SERVER_MTU);

	test_wait_for_l2cap_disconnect();

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_ERET, false,
					 BT_L2CAP_BR_LINK_MODE_ERET,
					 "l2cap_client::test_07_eret_mode::eret_mode");

	br_disconnect_from_peer();
}

ZTEST(l2cap_client, test_08_eret_mode_optional)
{
	br_connect_to_peer();
	zassert_true(br_conn != NULL, "Connect handle is NULL");

	sdp_uuid.uuid.type = BT_UUID_TYPE_16;
	sdp_uuid.u16.val = BT_SDP_PROTO_L2CAP;
	l2cap_psm = 0xffff;
	test_sdp_discover_service_search_attr(br_conn, &sdp_uuid.uuid,
					      test_sdp_discover_l2cap_chan_cb, NULL);

	test_l2cap_conn_failed(BT_L2CAP_BR_LINK_MODE_BASIC, false, BT_L2CAP_BR_LINK_MODE_BASIC,
			       "l2cap_client::test_08_eret_mode_optional::eret_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_ERET, false,
					 BT_L2CAP_BR_LINK_MODE_ERET,
					 "l2cap_client::test_08_eret_mode_optional::eret_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_BASIC, false,
					 BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_client::test_08_eret_mode_optional::basic_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_ERET, true,
					 BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_client::test_08_eret_mode_optional::basic_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_ERET, true,
					 BT_L2CAP_BR_LINK_MODE_ERET,
					 "l2cap_client::test_08_eret_mode_optional::eret_mode");

	br_disconnect_from_peer();
}

ZTEST(l2cap_client, test_09_stream_mode)
{
	int err;

	br_connect_to_peer();
	zassert_true(br_conn != NULL, "Connect handle is NULL");

	sdp_uuid.uuid.type = BT_UUID_TYPE_16;
	sdp_uuid.u16.val = BT_SDP_PROTO_L2CAP;
	l2cap_psm = 0xffff;
	test_sdp_discover_service_search_attr(br_conn, &sdp_uuid.uuid,
					      test_sdp_discover_l2cap_chan_cb, NULL);

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_STREAM, false,
					 BT_L2CAP_BR_LINK_MODE_STREAM,
					 "l2cap_client::test_09_stream_mode::stream_mode");

	test_l2cap_conn_disconn_sequence_with_len(BT_L2CAP_BR_LINK_MODE_STREAM, false,
						  BT_L2CAP_BR_LINK_MODE_STREAM,
						  CONFIG_TEST_L2CAP_CLIENT_MTU);

	test_l2cap_conn_create(BT_L2CAP_BR_LINK_MODE_STREAM, false);

	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_STREAM);
	for (int i = 0; i < CONFIG_TEST_L2CAP_CLIENT_TX_DATA_POOL_COUNT; i++) {
		test_l2cap_send_buf(CONFIG_TEST_L2CAP_SERVER_MTU);
	}

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
	zassert_true(err < 0, "RX should not be received");
#endif /* CONFIG_BT_L2CAP_RET_FC */
	test_recv_complete(CONFIG_TEST_L2CAP_CLIENT_MTU);

	err = bt_l2cap_chan_disconnect(&l2cap_chan.chan.chan);
	zassert_equal(err, 0, "L2CAP channel disconnection failed (err %d)", err);

	test_wait_for_l2cap_disconnect();

	atomic_set_bit(l2cap_chan.flags, TEST_L2CAP_FLAG_RX_DISABLED);
	test_l2cap_conn_create(BT_L2CAP_BR_LINK_MODE_STREAM, false);
	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_STREAM);

	test_wait_for_l2cap_disconnect();

	test_l2cap_conn_create(BT_L2CAP_BR_LINK_MODE_STREAM, false);
	test_wait_for_l2cap_connect(BT_L2CAP_BR_LINK_MODE_STREAM);

	test_l2cap_send_buf(CONFIG_TEST_L2CAP_SERVER_MTU);

	test_wait_for_l2cap_disconnect();

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_STREAM, false,
					 BT_L2CAP_BR_LINK_MODE_STREAM,
					 "l2cap_client::test_09_stream_mode::stream_mode");

	br_disconnect_from_peer();
}

ZTEST(l2cap_client, test_10_stream_mode_optional)
{
	br_connect_to_peer();
	zassert_true(br_conn != NULL, "Connect handle is NULL");

	sdp_uuid.uuid.type = BT_UUID_TYPE_16;
	sdp_uuid.u16.val = BT_SDP_PROTO_L2CAP;
	l2cap_psm = 0xffff;
	test_sdp_discover_service_search_attr(br_conn, &sdp_uuid.uuid,
					      test_sdp_discover_l2cap_chan_cb, NULL);

	test_l2cap_conn_failed(BT_L2CAP_BR_LINK_MODE_BASIC, false, BT_L2CAP_BR_LINK_MODE_BASIC,
			       "l2cap_client::test_10_stream_mode_optional::stream_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_STREAM, false,
					 BT_L2CAP_BR_LINK_MODE_STREAM,
					 "l2cap_client::test_10_stream_mode_optional::stream_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_BASIC, false,
					 BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_client::test_10_stream_mode_optional::basic_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_STREAM, true,
					 BT_L2CAP_BR_LINK_MODE_BASIC,
					 "l2cap_client::test_10_stream_mode_optional::basic_mode");

	test_l2cap_conn_disconn_sequence(BT_L2CAP_BR_LINK_MODE_STREAM, true,
					 BT_L2CAP_BR_LINK_MODE_STREAM,
					 "l2cap_client::test_10_stream_mode_optional::stream_mode");

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

	LOG_DBG("Initializing Bluetooth");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	zassert_equal(err, 0, "Bluetooth init failed (err %d)", err);

	LOG_DBG("Bluetooth initialized");

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
	atomic_clear_bit(test_flags, TEST_FLAG_DEVICE_FOUND);

	k_sem_reset(&br_connect_changed_sem);
	k_sem_reset(&br_discovery_found_sem);
	k_sem_reset(&br_sdp_discover_sem);
}

static void after(void *f)
{
	br_disconnect_from_peer();

	atomic_clear(l2cap_chan.flags);
}

ZTEST_SUITE(l2cap_client, NULL, setup, before, after, teardown);
