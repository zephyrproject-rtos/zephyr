/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(test_classic_rfcomm_sim, CONFIG_TEST_CLASSIC_RFCOMM_SIM_LOG_LEVEL);

#define TEST_RFCOMM_CHAN_CONNECT_TIMEOUT_S 10
#define TEST_RFCOMM_CHAN_RX_TIMEOUT_S 10
#define TEST_SDP_DISCOVERY_TIMEOUT_S 10

extern bt_addr_t peer_addr;
static K_SEM_DEFINE(rfcomm_state_changed_sem, 0, 1);

static struct bt_conn *br_conn;

struct bt_rfcomm_chan {
	struct bt_rfcomm_dlc dlc;
	struct k_fifo rx_fifo;
	atomic_t flags[1];
	struct k_sem sem;
};

#define RFCOMM_MTU 100

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_MAX_CONN, BT_RFCOMM_BUF_SIZE(RFCOMM_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static struct bt_rfcomm_server server[CONFIG_TEST_RFCOMM_MAX_CHANNEL_COUNT];
static struct bt_rfcomm_chan chan[CONFIG_TEST_RFCOMM_MAX_CHANNEL_COUNT];

#define _SDP_ATTRS_DEFINE(index, sdp_attrs_name) \
static struct bt_sdp_attribute sdp_attrs_name##index[] = { \
	BT_SDP_NEW_SERVICE, \
	BT_SDP_LIST( \
		BT_SDP_ATTR_SVCLASS_ID_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), \
		BT_SDP_DATA_ELEM_LIST( \
		{ \
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
			BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS) \
		}, \
		) \
	), \
	BT_SDP_LIST( \
		BT_SDP_ATTR_PROTO_DESC_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12), \
		BT_SDP_DATA_ELEM_LIST( \
		{ \
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), \
			BT_SDP_DATA_ELEM_LIST( \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) \
			}, \
			) \
		}, \
		{ \
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5), \
			BT_SDP_DATA_ELEM_LIST( \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
				BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM) \
			}, \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8), \
				&server[index].channel \
			}, \
			) \
		}, \
		) \
	), \
	BT_SDP_LIST( \
		BT_SDP_ATTR_PROFILE_DESC_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8), \
		BT_SDP_DATA_ELEM_LIST( \
		{ \
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6), \
			BT_SDP_DATA_ELEM_LIST( \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
				BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS) \
			}, \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16), \
				BT_SDP_ARRAY_16(0x0102) \
			}, \
			) \
		}, \
		) \
	), \
	BT_SDP_SERVICE_NAME("Serial Port"), \
}

#define SDP_ATTRS_DEFINE(index, ...) _SDP_ATTRS_DEFINE(index, ##__VA_ARGS__)

LISTIFY(CONFIG_TEST_RFCOMM_MAX_CHANNEL_COUNT, SDP_ATTRS_DEFINE, (;), rfcomm_attrs);

#define _SDP_REC_DEFINE(index, sdp_attrs_name) BT_SDP_RECORD(sdp_attrs_name##index)
#define SDP_REC_DEFINE(index, ...)             _SDP_REC_DEFINE(index, ##__VA_ARGS__)

static struct bt_sdp_record rfcomm_rec[CONFIG_TEST_RFCOMM_MAX_CHANNEL_COUNT] = {
	LISTIFY(CONFIG_TEST_RFCOMM_MAX_CHANNEL_COUNT, SDP_REC_DEFINE, (,), rfcomm_attrs),
};

static uint8_t discovered_server_channel[CONFIG_TEST_RFCOMM_MAX_CHANNEL_COUNT];
static uint8_t discovered_server_channel_count;

enum {
	TEST_FLAG_CONN_CONNECTED,
	TEST_FLAG_SDP_DISCOVERING,
	TEST_FLAG_SDP_DISCOVERY_DONE,
	TEST_FLAG_HOLD_CREDIT,
	TEST_FLAG_IN_USED,
	TEST_FLAG_DATA_RECVED,
	TEST_FLAG_INITIATOR,
};

static atomic_t test_flags[1];

static uint8_t rfcomm_sdp_discover_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
				      const struct bt_sdp_discover_params *params)
{
	int err;
	uint16_t param;

	if (result == NULL || result->resp_buf == NULL) {
		LOG_DBG("SDP discover done");
		goto done;
	}

	err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &param);
	if (err != 0) {
		LOG_ERR("Error getting Server channel (%d)", err);
		goto next;
	}

	LOG_DBG("Found rfcomm channel %u", param);
	if (discovered_server_channel_count < ARRAY_SIZE(discovered_server_channel)) {
		discovered_server_channel[discovered_server_channel_count] = param;
		discovered_server_channel_count++;
	}

next:
	if (result->next_record_hint) {
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}
done:
	atomic_set_bit(test_flags, TEST_FLAG_SDP_DISCOVERY_DONE);
	atomic_clear_bit(test_flags, TEST_FLAG_SDP_DISCOVERING);
	k_sem_give(&rfcomm_state_changed_sem);
	return BT_SDP_DISCOVER_UUID_STOP;
}

#define RFCOMM_SDP_BUF_LEN 512U

NET_BUF_POOL_FIXED_DEFINE(sdp_pool, CONFIG_BT_MAX_CONN, RFCOMM_SDP_BUF_LEN,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static void sdp_discover_work_handler(struct k_work *work)
{
	static struct bt_sdp_attribute_id_range id_range[] = {
		{ BT_SDP_ATTR_PROTO_DESC_LIST, BT_SDP_ATTR_PROTO_DESC_LIST },
		{ BT_SDP_ATTR_PROFILE_DESC_LIST, BT_SDP_ATTR_PROFILE_DESC_LIST },
	};
	static struct bt_sdp_attribute_id_list id_list = {
		.count = ARRAY_SIZE(id_range),
		.ranges = id_range,
	};
	static struct bt_sdp_discover_params sdp_param;
	static struct bt_uuid_16 uuid;
	int err;

	if (br_conn == NULL) {
		LOG_ERR("Invalid ACL conn");
		return;
	}

	if (!bt_conn_is_type(br_conn, BT_CONN_TYPE_BR)) {
		LOG_ERR("Invalid ACL conn type");
		return;
	}

	if (atomic_test_and_set_bit(test_flags, TEST_FLAG_SDP_DISCOVERING)) {
		LOG_ERR("Discovery is ongoing");
		return;
	}

	discovered_server_channel_count = 0;
	memset(&discovered_server_channel, 0, sizeof(discovered_server_channel));

	uuid.uuid.type = BT_UUID_TYPE_16;
	uuid.val = BT_SDP_SERIAL_PORT_SVCLASS;

	sdp_param.func = rfcomm_sdp_discover_cb;
	sdp_param.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;
	sdp_param.uuid = &uuid.uuid;
	sdp_param.pool = &sdp_pool;
	sdp_param.ids  = &id_list;

	err = bt_sdp_discover(br_conn, &sdp_param);
	if (err != 0) {
		LOG_ERR("SDP discovery failed (%d)", err);
		atomic_clear_bit(test_flags, TEST_FLAG_SDP_DISCOVERING);
		return;
	}
}

static K_WORK_DEFINE(sdp_discover_work, sdp_discover_work_handler);

static const char *rfcomm_get_role_str(const struct bt_rfcomm_chan *chan)
{
	bool initiator = atomic_test_bit(chan->flags, TEST_FLAG_INITIATOR);

	return initiator ? "initiator" : "acceptor";
}

static void chan_connected(struct bt_rfcomm_dlc *dlc)
{
	struct bt_rfcomm_chan *c = CONTAINER_OF(dlc, struct bt_rfcomm_chan, dlc);

	LOG_DBG("%s: DLC %zu connected", rfcomm_get_role_str(c), ARRAY_INDEX(chan, c));
	atomic_set_bit(c->flags, TEST_FLAG_CONN_CONNECTED);
	k_sem_give(&c->sem);
}

static void chan_disconnected(struct bt_rfcomm_dlc *dlc)
{
	struct net_buf *buf;
	struct bt_rfcomm_chan *c = CONTAINER_OF(dlc, struct bt_rfcomm_chan, dlc);

	LOG_DBG("%s: DLC %zu disconnected", rfcomm_get_role_str(c), ARRAY_INDEX(chan, c));

	buf = k_fifo_get(&c->rx_fifo, K_NO_WAIT);
	while (buf != NULL) {
		net_buf_unref(buf);
		buf = k_fifo_get(&c->rx_fifo, K_NO_WAIT);
	}
	atomic_clear_bit(c->flags, TEST_FLAG_CONN_CONNECTED);
	atomic_clear_bit(c->flags, TEST_FLAG_IN_USED);
	k_sem_give(&c->sem);
}

static int chan_recv(struct bt_rfcomm_dlc *dlc, struct net_buf *buf)
{
	struct bt_rfcomm_chan *c = CONTAINER_OF(dlc, struct bt_rfcomm_chan, dlc);

	LOG_DBG("%s: DLC %zu RX len %u", rfcomm_get_role_str(c), ARRAY_INDEX(chan, c), buf->len);

	atomic_set_bit(c->flags, TEST_FLAG_DATA_RECVED);

	if (!atomic_test_bit(c->flags, TEST_FLAG_HOLD_CREDIT)) {
		k_sem_give(&c->sem);
		return 0;
	}

	k_fifo_put(&c->rx_fifo, buf);

	k_sem_give(&c->sem);
	return -EINPROGRESS;
}

static struct bt_rfcomm_dlc_ops rfcomm_dlc_ops = {
	.connected = chan_connected,
	.disconnected = chan_disconnected,
	.recv = chan_recv,
};

static int accept(struct bt_conn *conn, struct bt_rfcomm_server *s, struct bt_rfcomm_dlc **dlc)
{
	struct bt_rfcomm_chan *c;

	if (ARRAY_INDEX(server, s) >= ARRAY_SIZE(chan)) {
		LOG_WRN("No more rfcomm DLC");
		return -ENOMEM;
	}

	c = &chan[ARRAY_INDEX(server, s)];
	if (atomic_test_and_set_bit(c->flags, TEST_FLAG_IN_USED)) {
		LOG_WRN("DLC %p is used", &c->dlc);
		return -EBUSY;
	}

	c->dlc.rx_credit_limit = 1;
	c->dlc.ops = &rfcomm_dlc_ops;
	c->dlc.mtu = CONFIG_BT_RFCOMM_L2CAP_MTU;
	c->dlc.required_sec_level = BT_SECURITY_L2;
	k_fifo_init(&c->rx_fifo);
	*dlc = &c->dlc;
	return 0;
}

static void br_connected(struct bt_conn *conn, uint8_t conn_err)
{
	LOG_DBG("connected: conn %p err 0x%02x", (void *)conn, conn_err);
	if (conn_err == 0 && br_conn == NULL) {
		br_conn = bt_conn_ref(conn);
		atomic_set_bit(test_flags, TEST_FLAG_CONN_CONNECTED);
	} else {
		LOG_ERR("Connection failed");
	}
	k_sem_give(&rfcomm_state_changed_sem);
}

static void br_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_DBG("disconnected: conn %p reason 0x%02x", (void *)conn, reason);
	if (br_conn != NULL) {
		bt_conn_unref(br_conn);
		br_conn = NULL;
		atomic_clear_bit(test_flags, TEST_FLAG_CONN_CONNECTED);
	}
	k_sem_give(&rfcomm_state_changed_sem);
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

	LOG_DBG("Connecting to peer device");
	k_sem_reset(&rfcomm_state_changed_sem);
	conn = bt_conn_create_br(&peer_addr, BT_BR_CONN_PARAM_DEFAULT);
	zassert_true(conn != NULL, "BR connection creating failed");

	err = k_sem_take(&rfcomm_state_changed_sem, K_SECONDS(CONFIG_BT_CREATE_CONN_TIMEOUT));
	zassert_equal(err, 0, "Connection timeout (err %d)", err);
	zassert_true(br_conn != NULL, "Connect handle is NULL");
	zassert_equal(br_conn, conn, "Connect mismatched %p != %p", br_conn, conn);

	LOG_DBG("BR connection established");
	bt_conn_unref(conn);

	(void)k_work_submit(&sdp_discover_work);
	err = k_sem_take(&rfcomm_state_changed_sem, K_SECONDS(TEST_SDP_DISCOVERY_TIMEOUT_S));
	zassert_equal(err, 0, "SDP discovery timeout (err %d)", err);
	zassert_true(atomic_test_bit(test_flags, TEST_FLAG_SDP_DISCOVERY_DONE),
		     "SDP discovery failed");
}

static void br_disconnect_from_peer(void)
{
	int err;

	if (br_conn == NULL) {
		return;
	}

	LOG_DBG("Disconnecting from peer device");
	k_sem_reset(&rfcomm_state_changed_sem);
	err = bt_conn_disconnect(br_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	zassert_equal(err, 0, "Failed to disconnect (err %d)", err);

	err = k_sem_take(&rfcomm_state_changed_sem, K_SECONDS(30));
	zassert_equal(err, 0, "Disconnection timeout (err %d)", err);
	zassert_equal(br_conn, NULL, "br_conn is not NULL after disconnect");
}

static void test_wait_for_connect(void)
{
	int err;

	err = k_sem_take(&rfcomm_state_changed_sem, K_SECONDS(CONFIG_BT_CREATE_CONN_TIMEOUT));
	zassert_equal(err, 0, "Connection timeout (err %d)", err);
	zassert_true(atomic_test_bit(test_flags, TEST_FLAG_CONN_CONNECTED), "Connection failed");
}

static void test_wait_for_disconnect(k_timeout_t timeout)
{
	int err;

	err = k_sem_take(&rfcomm_state_changed_sem, timeout);
	zassert_equal(err, 0, "Disconnection timeout (err %d)", err);
	zassert_false(atomic_test_bit(test_flags, TEST_FLAG_CONN_CONNECTED),
		      "Disconnection failed");
}

static void rfcomm_chan_init(size_t index)
{
	int err;

	err = k_sem_init(&chan[index].sem, 0, 1);
	zassert_equal(err, 0, "Failed to init sem (err %d)", err);

	atomic_clear(chan[index].flags);
}

static void rfcomm_chans_init(void)
{
	ARRAY_FOR_EACH(chan, index) {
		rfcomm_chan_init(index);
	}
}

static void rfcomm_server_init(size_t index)
{
	int err;

	server[index].accept = accept;
	err = bt_rfcomm_server_register(&server[index]);
	zassert_equal(err, 0, "Failed to register server (err %d)", err);

	err = bt_sdp_register_service(&rfcomm_rec[index]);
	zassert_equal(err, 0, "Failed to register SDP record (err %d)", err);
}

static void rfcomm_servers_init(void)
{
	ARRAY_FOR_EACH(server, index) {
		rfcomm_server_init(index);
	}
}

static void rfcomm_set_connectable(void)
{
	int err;

	err = bt_br_set_connectable(true, NULL);
	zassert_equal(err, 0, "Failed to set connectable (err %d)", err);

	err = bt_br_set_discoverable(true, true);
	zassert_equal(err, 0, "Failed to set discoverable (err %d)", err);
}

static void rfcomm_chan_connect(size_t index)
{
	int err;
	uint8_t channel;

	zassert_between_inclusive(index, 0, discovered_server_channel_count - 1,
				  "channel ID is not found");

	channel = discovered_server_channel[index];
	zassert_not_equal(channel, 0, "Invalid channel id");

	atomic_set_bit(chan[index].flags, TEST_FLAG_IN_USED);
	atomic_set_bit(chan[index].flags, TEST_FLAG_INITIATOR);
	chan[index].dlc.rx_credit_limit = 1;
	chan[index].dlc.ops = &rfcomm_dlc_ops;
	chan[index].dlc.mtu = CONFIG_BT_RFCOMM_L2CAP_MTU;
	chan[index].dlc.required_sec_level = BT_SECURITY_L2;
	k_fifo_init(&chan[index].rx_fifo);

	err = bt_rfcomm_dlc_connect(br_conn, &chan[index].dlc, channel);
	zassert_equal(err, 0, "Failed to create DLC conn %zu (err %d)", index, err);
}

static void rfcomm_chans_connect(void)
{
	ARRAY_FOR_EACH(chan, index) {
		rfcomm_chan_connect(index);
	}
}

static void rfcomm_chan_disconnect(size_t index)
{
	int err;

	zassert_true(atomic_test_bit(chan[index].flags, TEST_FLAG_CONN_CONNECTED),
		     "RFCOMM chan %zu is not connected", index);

	err = bt_rfcomm_dlc_disconnect(&chan[index].dlc);
	zassert_equal(err, 0, "Failed to disconn DLC %zu (err %d)", index, err);
}

static void rfcomm_chans_disconnect(void)
{
	ARRAY_FOR_EACH(chan, index) {
		rfcomm_chan_disconnect(index);
	}
}

static void rfcomm_wait_for_chan_connect(size_t index)
{
	int err;

	err = k_sem_take(&chan[index].sem, K_SECONDS(TEST_RFCOMM_CHAN_CONNECT_TIMEOUT_S));
	zassert_equal(err, 0, "RFCOMM chan %zu connection timeout (err %d)", index, err);
	zassert_true(atomic_test_bit(chan[index].flags, TEST_FLAG_CONN_CONNECTED),
		     "RFCOMM chan %zu connection failed", index);
}

static void rfcomm_wait_for_chans_connect(void)
{
	int err;

	ARRAY_FOR_EACH(chan, index) {
		err = k_sem_take(&chan[index].sem, K_SECONDS(TEST_RFCOMM_CHAN_CONNECT_TIMEOUT_S));
		zassert_equal(err, 0, "RFCOMM chan %zu connection timeout (err %d)", index, err);
		zassert_true(atomic_test_bit(chan[index].flags, TEST_FLAG_CONN_CONNECTED),
			     "RFCOMM chan %zu connection failed", index);
	}
}

static void rfcomm_wait_for_data(size_t index)
{
	int err;

	err = k_sem_take(&chan[index].sem, K_SECONDS(TEST_RFCOMM_CHAN_RX_TIMEOUT_S));
	zassert_equal(err, 0, "RFCOMM chan %zu rx timeout (err %d)", index, err);
	zassert_true(atomic_test_and_clear_bit(chan[index].flags, TEST_FLAG_DATA_RECVED),
		     "RFCOMM chan %zu rx failed", index);
}

static void rfcomm_wait_for_no_data(size_t index, k_timeout_t timeout)
{
	int err;

	err = k_sem_take(&chan[index].sem, timeout);
	zassert_not_equal(err, 0, "RFCOMM chan %zu rx waiting should be timeout", index);
	zassert_false(atomic_test_bit(chan[index].flags, TEST_FLAG_DATA_RECVED),
		      "RFCOMM chan %zu rx callback should not be triggered", index);
}

static void rfcomm_wait_for_chan_disconnect(size_t index, k_timeout_t timeout)
{
	int err;

	err = k_sem_take(&chan[index].sem, timeout);
	zassert_equal(err, 0, "RFCOMM chan %zu disconnection timeout (err %d)", index, err);
	zassert_false(atomic_test_bit(chan[index].flags, TEST_FLAG_CONN_CONNECTED),
		      "RFCOMM chan %zu disconnection failed", index);
}

static void rfcomm_wait_for_chans_disconnect(k_timeout_t timeout)
{
	int err;

	ARRAY_FOR_EACH(chan, index) {
		err = k_sem_take(&chan[index].sem, timeout);
		zassert_equal(err, 0, "RFCOMM chan %zu disconnection timeout (err %d)", index, err);
		zassert_false(atomic_test_bit(chan[index].flags, TEST_FLAG_CONN_CONNECTED),
			      "RFCOMM chan %zu disconnection failed", index);
	}
}

static void rfcomm_send_data(size_t index)
{
	struct net_buf *buf;
	int err;

	buf = bt_rfcomm_create_pdu(&tx_pool);
	zassume_not_null(buf, "Failed to allocate buffer for RFCOMM chan %zu", index);

	(void)net_buf_add_u8(buf, (uint8_t)index);

	err = bt_rfcomm_dlc_send(&chan[index].dlc, buf);
	zassert_true(err > 0, "Failed to send data for RFCOMM chan %zu (err %d)", index, err);
}

static void rfcomm_give_credit(size_t index)
{
	int err;
	struct net_buf *buf;

	buf = k_fifo_get(&chan[index].rx_fifo, K_NO_WAIT);
	zassume_not_null(buf, "Not buffer in using of RFCOMM chan %zu", index);

	err = bt_rfcomm_dlc_recv_complete(&chan[index].dlc, buf);
	zassert_equal(err, 0, "Failed to give credit on chan %zu (err %d)", index, err);
}

ZTEST(rfcomm_client, test_01_single_conn)
{
	rfcomm_chan_init(0);

	br_connect_to_peer();

	rfcomm_chan_connect(0);
	rfcomm_wait_for_chan_connect(0);

	rfcomm_wait_for_data(0);
	rfcomm_send_data(0);
	rfcomm_wait_for_data(0);

	rfcomm_chan_disconnect(0);
	rfcomm_wait_for_chan_disconnect(0, K_SECONDS(30));
	br_disconnect_from_peer();
}

ZTEST(rfcomm_server, test_01_single_conn)
{
	rfcomm_server_init(0);
	rfcomm_chan_init(0);
	rfcomm_set_connectable();

	test_wait_for_connect();

	rfcomm_wait_for_chan_connect(0);

	rfcomm_send_data(0);
	rfcomm_wait_for_data(0);
	rfcomm_send_data(0);

	rfcomm_wait_for_chan_disconnect(0, K_SECONDS(30));

	test_wait_for_disconnect(K_SECONDS(30));
}

ZTEST(rfcomm_client, test_02_single_conn_with_credit)
{
	rfcomm_chan_init(0);

	br_connect_to_peer();

	atomic_set_bit(chan[0].flags, TEST_FLAG_HOLD_CREDIT);
	rfcomm_chan_connect(0);
	rfcomm_wait_for_chan_connect(0);

	rfcomm_wait_for_data(0);
	rfcomm_send_data(0);
	rfcomm_wait_for_no_data(0, K_SECONDS(3));
	rfcomm_give_credit(0);
	rfcomm_wait_for_data(0);
	rfcomm_give_credit(0);
	rfcomm_send_data(0);
	rfcomm_wait_for_data(0);
	rfcomm_give_credit(0);

	rfcomm_chan_disconnect(0);
	rfcomm_wait_for_chan_disconnect(0, K_SECONDS(30));
	br_disconnect_from_peer();
}

ZTEST(rfcomm_server, test_02_single_conn_with_credit)
{
	rfcomm_server_init(0);
	rfcomm_chan_init(0);
	atomic_set_bit(chan[0].flags, TEST_FLAG_HOLD_CREDIT);
	rfcomm_set_connectable();

	test_wait_for_connect();

	rfcomm_wait_for_chan_connect(0);
	rfcomm_send_data(0);
	rfcomm_wait_for_data(0);
	rfcomm_send_data(0);
	rfcomm_wait_for_no_data(0, K_SECONDS(3));
	rfcomm_give_credit(0);
	rfcomm_wait_for_data(0);
	rfcomm_give_credit(0);
	rfcomm_send_data(0);

	rfcomm_wait_for_chan_disconnect(0, K_SECONDS(30));

	test_wait_for_disconnect(K_SECONDS(30));
}

ZTEST(rfcomm_client, test_03_multi_conn)
{
	rfcomm_chans_init();

	br_connect_to_peer();

	rfcomm_chans_connect();
	rfcomm_wait_for_chans_connect();

	rfcomm_wait_for_data(0);
	rfcomm_send_data(0);
	rfcomm_wait_for_data(0);

	rfcomm_chans_disconnect();
	rfcomm_wait_for_chans_disconnect(K_SECONDS(30));
	br_disconnect_from_peer();
}

ZTEST(rfcomm_server, test_03_multi_conn)
{
	rfcomm_servers_init();
	rfcomm_chans_init();
	rfcomm_set_connectable();

	test_wait_for_connect();

	rfcomm_wait_for_chans_connect();

	rfcomm_send_data(0);
	rfcomm_wait_for_data(0);
	rfcomm_send_data(0);

	rfcomm_wait_for_chans_disconnect(K_SECONDS(30));

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

ZTEST_SUITE(rfcomm_client, NULL, setup, NULL, NULL, teardown);
ZTEST_SUITE(rfcomm_server, NULL, setup, NULL, NULL, teardown);
