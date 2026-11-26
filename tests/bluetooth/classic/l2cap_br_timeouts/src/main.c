/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define DT_DRV_COMPAT zephyr_bt_hci_test

#define ACL_HANDLE           1
#define SIGNAL_CID           1
#define PEER_CID             0x0041
#define TEST_PSM             0x1001
#define PEER_CONFIG_IDENT    0x80
#define PEER_INFO_IDENT      0x81
#define RFC_OPTION           0x04
#define RFC_SIZE             9
#define PEER_RET_TIMEOUT     3000
#define PEER_MONITOR_TIMEOUT 15000

enum signal_code {
	CONNECTION_REQUEST = 0x02,
	CONNECTION_RESPONSE = 0x03,
	CONFIGURATION_REQUEST = 0x04,
	CONFIGURATION_RESPONSE = 0x05,
	DISCONNECTION_REQUEST = 0x06,
	DISCONNECTION_RESPONSE = 0x07,
	INFORMATION_REQUEST = 0x0a,
	INFORMATION_RESPONSE = 0x0b,
};

static const bt_addr_t peer_address = {{0x06, 0x05, 0x04, 0x03, 0x02, 0x01}};

static struct {
	uint8_t mode;
	bool reject_once;
	uint16_t request_ret;
	uint16_t request_monitor;
	uint16_t local_cid;
	size_t request_count;
	uint16_t sent_ret[2];
	uint16_t sent_monitor[2];
	uint16_t response_result;
	uint16_t response_ret;
	uint16_t response_monitor;
	int64_t iframe_time;
	int64_t poll_time[2];
	size_t poll_count;
} peer;

static struct {
	struct bt_conn *conn;
	struct bt_l2cap_br_chan channel;
	bool channel_started;
} fixture;

static K_SEM_DEFINE(link_connected, 0, 1);
static K_SEM_DEFINE(link_disconnected, 0, 1);
static K_SEM_DEFINE(features_ready, 0, 1);
static K_SEM_DEFINE(channel_connected, 0, 1);
static K_SEM_DEFINE(channel_released, 0, 1);
static K_SEM_DEFINE(config_response, 0, 1);
static K_SEM_DEFINE(iframe_sent, 0, 1);
static K_SEM_DEFINE(poll_sent, 0, 2);

NET_BUF_POOL_FIXED_DEFINE(data_pool, 1, BT_L2CAP_BUF_SIZE(1), CONFIG_BT_CONN_TX_USER_DATA_SIZE,
			  NULL);

static void receive_from_controller(const struct device *dev, struct net_buf *buf)
{
	int err = bt_hci_recv_err(dev, buf);

	if (err != 0) {
		net_buf_unref(buf);
	}
	zassert_ok(err, "Host rejected controller packet: %d", err);
}

static void send_event(const struct device *dev, uint8_t code, const void *data, size_t len)
{
	struct net_buf *buf = bt_buf_get_evt(code, false, K_FOREVER);
	struct bt_hci_evt_hdr *hdr;

	zassert_not_null(buf);
	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = code;
	hdr->len = len;
	net_buf_add_mem(buf, data, len);
	receive_from_controller(dev, buf);
}

static void complete_command(const struct device *dev, uint16_t opcode, const void *data,
			     size_t len)
{
	NET_BUF_SIMPLE_DEFINE(params, 80);

	net_buf_simple_add_u8(&params, 1);
	net_buf_simple_add_le16(&params, opcode);
	net_buf_simple_add_mem(&params, data, len);
	send_event(dev, BT_HCI_EVT_CMD_COMPLETE, params.data, params.len);
}

static void command_status(const struct device *dev, uint16_t opcode)
{
	struct bt_hci_evt_cmd_status status = {
		.status = BT_HCI_ERR_SUCCESS,
		.ncmd = 1,
		.opcode = sys_cpu_to_le16(opcode),
	};

	send_event(dev, BT_HCI_EVT_CMD_STATUS, &status, sizeof(status));
}

static void handle_command(const struct device *dev, struct net_buf *buf)
{
	uint16_t opcode = net_buf_pull_le16(buf);
	uint8_t len = net_buf_pull_u8(buf);
	union {
		uint8_t status;
		struct bt_hci_rp_read_local_features features;
		struct bt_hci_rp_read_local_version_info version;
		struct bt_hci_rp_read_supported_commands commands;
		struct bt_hci_rp_read_bd_addr address;
		struct bt_hci_rp_read_buffer_size buffers;
		struct bt_hci_rp_le_read_local_features le_features;
		struct bt_hci_rp_le_read_buffer_size le_buffers;
		struct bt_hci_rp_read_default_link_policy_settings policy;
		struct bt_hci_rp_le_rand random;
	} response;
	size_t response_len = sizeof(response.status);

	zassert_equal(len, buf->len);
	memset(&response, 0, sizeof(response));

	switch (opcode) {
	case BT_HCI_OP_READ_LOCAL_FEATURES:
		response.features.features[0] = BIT(5);
		response.features.features[4] = BIT(6);
		response.features.features[6] = BIT(0) | BIT(3);
		response_len = sizeof(response.features);
		break;
	case BT_HCI_OP_READ_LOCAL_VERSION_INFO:
		response.version.hci_version = BT_HCI_VERSION_5_0;
		response.version.lmp_version = BT_HCI_VERSION_5_0;
		response.version.manufacturer = sys_cpu_to_le16(0xffff);
		response_len = sizeof(response.version);
		break;
	case BT_HCI_OP_READ_SUPPORTED_COMMANDS:
		response_len = sizeof(response.commands);
		break;
	case BT_HCI_OP_READ_BD_ADDR:
		response.address.bdaddr.val[0] = 0x42;
		response_len = sizeof(response.address);
		break;
	case BT_HCI_OP_READ_BUFFER_SIZE:
		response.buffers.acl_max_len = sys_cpu_to_le16(128);
		response.buffers.acl_max_num = sys_cpu_to_le16(4);
		response_len = sizeof(response.buffers);
		break;
	case BT_HCI_OP_LE_READ_LOCAL_FEATURES:
		response.le_features.features[0] = BIT(0);
		response_len = sizeof(response.le_features);
		break;
	case BT_HCI_OP_LE_READ_BUFFER_SIZE:
		response_len = sizeof(response.le_buffers);
		break;
	case BT_HCI_OP_READ_DEFAULT_LINK_POLICY_SETTINGS:
		response_len = sizeof(response.policy);
		break;
	case BT_HCI_OP_LE_RAND:
		sys_rand_get(response.random.rand, sizeof(response.random.rand));
		response_len = sizeof(response.random);
		break;
	case BT_HCI_OP_CONNECT: {
		struct bt_hci_evt_conn_complete connected = {
			.handle = sys_cpu_to_le16(ACL_HANDLE),
			.link_type = BT_HCI_ACL,
		};

		zassert_true(buf->len >= sizeof(bt_addr_t));
		memcpy(&connected.bdaddr, buf->data, sizeof(bt_addr_t));
		command_status(dev, opcode);
		send_event(dev, BT_HCI_EVT_CONN_COMPLETE, &connected, sizeof(connected));
		return;
	}
	case BT_HCI_OP_READ_REMOTE_FEATURES: {
		struct bt_hci_evt_remote_features features = {
			.handle = sys_cpu_to_le16(ACL_HANDLE),
		};

		command_status(dev, opcode);
		send_event(dev, BT_HCI_EVT_REMOTE_FEATURES, &features, sizeof(features));
		return;
	}
	case BT_HCI_OP_DISCONNECT: {
		const struct bt_hci_cp_disconnect *cmd = (const void *)buf->data;
		struct bt_hci_evt_disconn_complete disconnected = {
			.handle = sys_cpu_to_le16(ACL_HANDLE),
		};

		zassert_equal(buf->len, sizeof(*cmd));
		disconnected.reason = cmd->reason;
		command_status(dev, opcode);
		send_event(dev, BT_HCI_EVT_DISCONN_COMPLETE, &disconnected, sizeof(disconnected));
		return;
	}
	case BT_HCI_OP_RESET:
	case BT_HCI_OP_SET_EVENT_MASK:
	case BT_HCI_OP_LE_SET_EVENT_MASK:
	case BT_HCI_OP_LE_WRITE_LE_HOST_SUPP:
	case BT_HCI_OP_WRITE_SSP_MODE:
	case BT_HCI_OP_WRITE_INQUIRY_MODE:
	case BT_HCI_OP_WRITE_LOCAL_NAME:
	case BT_HCI_OP_WRITE_CLASS_OF_DEVICE:
	case BT_HCI_OP_WRITE_PAGE_TIMEOUT:
	case BT_HCI_OP_WRITE_DEFAULT_LINK_POLICY_SETTINGS:
	case BT_HCI_OP_WRITE_SCAN_ENABLE:
		break;
	default:
		zassert_unreachable("Unsupported controller command 0x%04x", opcode);
	}

	complete_command(dev, opcode, &response, response_len);
}

static void send_signal(const struct device *dev, uint8_t code, uint8_t ident,
			const struct net_buf_simple *payload)
{
	struct net_buf *buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);

	zassert_not_null(buf);
	net_buf_add_le16(buf, ACL_HANDLE | (BT_ACL_START << 12));
	net_buf_add_le16(buf, 8 + payload->len);
	net_buf_add_le16(buf, 4 + payload->len);
	net_buf_add_le16(buf, SIGNAL_CID);
	net_buf_add_u8(buf, code);
	net_buf_add_u8(buf, ident);
	net_buf_add_le16(buf, payload->len);
	net_buf_add_mem(buf, payload->data, payload->len);
	receive_from_controller(dev, buf);
}

static void add_rfc(struct net_buf_simple *buf, uint16_t ret, uint16_t monitor)
{
	net_buf_simple_add_u8(buf, RFC_OPTION);
	net_buf_simple_add_u8(buf, RFC_SIZE);
	net_buf_simple_add_u8(buf, peer.mode);
	net_buf_simple_add_u8(buf, 1);
	net_buf_simple_add_u8(buf, 3);
	net_buf_simple_add_le16(buf, ret);
	net_buf_simple_add_le16(buf, monitor);
	net_buf_simple_add_le16(buf, 64);
}

static const uint8_t *find_rfc(const uint8_t *data, size_t len)
{
	while (len > 0) {
		uint8_t size;

		zassert_true(len >= 2, "Truncated option header");
		size = data[1];
		zassert_true(len >= 2 + size, "Truncated option");
		if ((data[0] & 0x7f) == RFC_OPTION) {
			zassert_equal(size, RFC_SIZE);
			return data + 2;
		}
		data += 2 + size;
		len -= 2 + size;
	}

	zassert_unreachable("Missing RFC option");
	return NULL;
}

static void handle_signal(const struct device *dev, uint8_t code, uint8_t ident,
			  const uint8_t *data, size_t len)
{
	NET_BUF_SIMPLE_DEFINE(reply, 32);
	const uint8_t *rfc;

	switch (code) {
	case INFORMATION_REQUEST: {
		uint16_t type;

		zassert_equal(len, 2);
		type = sys_get_le16(data);
		net_buf_simple_add_le16(&reply, type);
		net_buf_simple_add_le16(&reply, 0);
		if (type == 2) {
			uint32_t features = BIT(0) | BIT(1) | BIT(4) | BIT(5) | BIT(7);

			if (peer.mode == BT_L2CAP_BR_LINK_MODE_ERET) {
				features |= BIT(3);
			}
			net_buf_simple_add_le32(&reply, features);
		} else if (type == 3) {
			net_buf_simple_add_le64(&reply, BIT64(SIGNAL_CID));
		} else {
			zassert_unreachable("Unexpected information type %u", type);
		}
		send_signal(dev, INFORMATION_RESPONSE, ident, &reply);
		if (type == 3) {
			/* A reply proves the Host consumed the preceding feature responses. */
			net_buf_simple_reset(&reply);
			net_buf_simple_add_le16(&reply, 2);
			send_signal(dev, INFORMATION_REQUEST, PEER_INFO_IDENT, &reply);
		}
		break;
	}
	case INFORMATION_RESPONSE:
		zassert_equal(ident, PEER_INFO_IDENT);
		zassert_equal(len, 8);
		zassert_equal(sys_get_le16(data), 2);
		zassert_equal(sys_get_le16(data + 2), 0);
		k_sem_give(&features_ready);
		break;
	case CONNECTION_REQUEST:
		zassert_equal(len, 4);
		zassert_equal(sys_get_le16(data), TEST_PSM);
		peer.local_cid = sys_get_le16(data + 2);
		net_buf_simple_add_le16(&reply, PEER_CID);
		net_buf_simple_add_le16(&reply, peer.local_cid);
		net_buf_simple_add_le16(&reply, 0);
		net_buf_simple_add_le16(&reply, 0);
		send_signal(dev, CONNECTION_RESPONSE, ident, &reply);

		net_buf_simple_reset(&reply);
		net_buf_simple_add_le16(&reply, peer.local_cid);
		net_buf_simple_add_le16(&reply, 0);
		add_rfc(&reply, peer.request_ret, peer.request_monitor);
		send_signal(dev, CONFIGURATION_REQUEST, PEER_CONFIG_IDENT, &reply);
		break;
	case CONFIGURATION_REQUEST:
		zassert_true(len >= 4);
		zassert_equal(sys_get_le16(data), PEER_CID);
		rfc = find_rfc(data + 4, len - 4);
		zassert_true(peer.request_count < ARRAY_SIZE(peer.sent_ret));
		peer.sent_ret[peer.request_count] = sys_get_le16(rfc + 3);
		peer.sent_monitor[peer.request_count] = sys_get_le16(rfc + 5);
		peer.request_count++;

		net_buf_simple_add_le16(&reply, peer.local_cid);
		net_buf_simple_add_le16(&reply, 0);
		net_buf_simple_add_le16(&reply,
					peer.reject_once && peer.request_count == 1 ? 1 : 0);
		add_rfc(&reply, PEER_RET_TIMEOUT, PEER_MONITOR_TIMEOUT);
		send_signal(dev, CONFIGURATION_RESPONSE, ident, &reply);
		break;
	case CONFIGURATION_RESPONSE:
		zassert_true(len >= 6);
		zassert_equal(ident, PEER_CONFIG_IDENT);
		peer.response_result = sys_get_le16(data + 4);
		rfc = find_rfc(data + 6, len - 6);
		peer.response_ret = sys_get_le16(rfc + 3);
		peer.response_monitor = sys_get_le16(rfc + 5);
		k_sem_give(&config_response);
		break;
	case DISCONNECTION_REQUEST:
		zassert_equal(len, 4);
		net_buf_simple_add_mem(&reply, data, len);
		send_signal(dev, DISCONNECTION_RESPONSE, ident, &reply);
		break;
	default:
		zassert_unreachable("Unexpected signaling code 0x%02x", code);
	}
}

static void handle_acl(const struct device *dev, struct net_buf *buf)
{
	NET_BUF_SIMPLE_DEFINE(completed, 5);
	uint16_t handle = net_buf_pull_le16(buf);
	uint16_t acl_len = net_buf_pull_le16(buf);
	uint16_t l2cap_len;
	uint16_t cid;

	zassert_equal(handle & 0x0fff, ACL_HANDLE);
	zassert_equal(acl_len, buf->len);
	l2cap_len = net_buf_pull_le16(buf);
	cid = net_buf_pull_le16(buf);
	zassert_equal(l2cap_len, buf->len);

	net_buf_simple_add_u8(&completed, 1);
	net_buf_simple_add_le16(&completed, ACL_HANDLE);
	net_buf_simple_add_le16(&completed, 1);
	send_event(dev, BT_HCI_EVT_NUM_COMPLETED_PACKETS, completed.data, completed.len);

	if (cid == SIGNAL_CID) {
		while (buf->len > 0) {
			uint8_t code = net_buf_pull_u8(buf);
			uint8_t ident = net_buf_pull_u8(buf);
			uint16_t len = net_buf_pull_le16(buf);
			const uint8_t *data = net_buf_pull_mem(buf, len);

			handle_signal(dev, code, ident, data, len);
		}
	} else {
		uint16_t control;

		zassert_equal(cid, PEER_CID);
		zassert_true(buf->len >= 2);
		control = sys_get_le16(buf->data);
		if ((control & BIT(0)) == 0) {
			peer.iframe_time = k_uptime_get();
			k_sem_give(&iframe_sent);
		} else {
			zassert_equal(control & 0x1f, BIT(0) | BIT(4), "Expected RR Poll");
			zassert_true(peer.poll_count < ARRAY_SIZE(peer.poll_time));
			peer.poll_time[peer.poll_count++] = k_uptime_get();
			k_sem_give(&poll_sent);
		}
	}
}

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = net_buf_pull_u8(buf);

	if (type == BT_HCI_H4_CMD) {
		handle_command(dev, buf);
	} else {
		zassert_equal(type, BT_HCI_H4_ACL);
		handle_acl(dev, buf);
	}
	net_buf_unref(buf);
	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.send = driver_send,
};

static struct bt_hci_driver_data driver_data;
static const struct bt_hci_driver_config driver_config = BT_DT_HCI_DRIVER_CONFIG_INST_GET(0);

DEVICE_DT_INST_DEFINE(0, NULL, NULL, &driver_data, &driver_config, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api);

static void connected(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(conn);
	zassert_equal(err, 0);
	k_sem_give(&link_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(reason);
	k_sem_give(&link_disconnected);
}

BT_CONN_CB_DEFINE(connection_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void channel_ready(struct bt_l2cap_chan *chan)
{
	ARG_UNUSED(chan);
	k_sem_give(&channel_connected);
}

static void channel_free(struct bt_l2cap_chan *chan)
{
	ARG_UNUSED(chan);
	k_sem_give(&channel_released);
}

static int channel_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	ARG_UNUSED(chan);
	ARG_UNUSED(buf);
	zassert_unreachable("Unexpected peer SDU");
	return 0;
}

static const struct bt_l2cap_chan_ops channel_ops = {
	.connected = channel_ready,
	.released = channel_free,
	.recv = channel_recv,
};

static void close_link(void)
{
	if (fixture.conn == NULL) {
		return;
	}
	zassert_ok(bt_conn_disconnect(fixture.conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN));
	zassert_ok(k_sem_take(&link_disconnected, K_SECONDS(5)), "Missing link disconnection");
	if (fixture.channel_started) {
		zassert_ok(k_sem_take(&channel_released, K_SECONDS(5)), "Channel not released");
		fixture.channel_started = false;
	}
	bt_conn_unref(fixture.conn);
	fixture.conn = NULL;
}

static void connect_channel(uint8_t mode, uint16_t ret, uint16_t monitor, bool retry)
{
	memset(&peer, 0, sizeof(peer));
	memset(&fixture.channel, 0, sizeof(fixture.channel));
	k_sem_reset(&link_connected);
	k_sem_reset(&link_disconnected);
	k_sem_reset(&features_ready);
	k_sem_reset(&channel_connected);
	k_sem_reset(&channel_released);
	k_sem_reset(&config_response);
	k_sem_reset(&iframe_sent);
	k_sem_reset(&poll_sent);

	peer.mode = mode;
	peer.request_ret = ret;
	peer.request_monitor = monitor;
	peer.reject_once = retry;
	fixture.channel.chan.ops = &channel_ops;
	fixture.channel.rx.mode = mode;
	fixture.channel.rx.mtu = 64;
	fixture.channel.required_sec_level = BT_SECURITY_L1;

	fixture.conn = bt_conn_create_br(&peer_address, BT_BR_CONN_PARAM_DEFAULT);
	zassert_not_null(fixture.conn);
	zassert_ok(k_sem_take(&link_connected, K_SECONDS(5)), "Missing BR connection");
	zassert_ok(k_sem_take(&features_ready, K_SECONDS(5)), "Feature exchange not completed");
	zassert_ok(bt_l2cap_chan_connect(fixture.conn, &fixture.channel.chan, TEST_PSM));
	fixture.channel_started = true;
	zassert_ok(k_sem_take(&config_response, K_SECONDS(5)), "Missing configuration response");
}

static void wait_channel(void)
{
	zassert_ok(k_sem_take(&channel_connected, K_SECONDS(5)), "Channel not connected");
}

static void *setup(void)
{
	zassert_ok(bt_enable(NULL));
	return NULL;
}

static void after(void *data)
{
	ARG_UNUSED(data);
	close_link();
}

ZTEST(l2cap_br_timeouts, test_ertm_initial_and_retry_requests_have_zero_timeouts)
{
	connect_channel(BT_L2CAP_BR_LINK_MODE_ERET, 0, 0, true);
	wait_channel();

	zassert_equal(peer.request_count, 2, "Expected one configuration retry");
	for (size_t i = 0; i < peer.request_count; i++) {
		zassert_equal(peer.sent_ret[i], 0);
		zassert_equal(peer.sent_monitor[i], 0);
	}
}

ZTEST(l2cap_br_timeouts, test_ertm_request_does_not_choose_local_timeouts)
{
	const uint16_t values[][2] = {
		{0, 0},
		{PEER_RET_TIMEOUT, PEER_MONITOR_TIMEOUT},
		{PEER_RET_TIMEOUT, 1},
	};

	for (size_t i = 0; i < ARRAY_SIZE(values); i++) {
		connect_channel(BT_L2CAP_BR_LINK_MODE_ERET, values[i][0], values[i][1], false);
		zassert_equal(peer.response_result, 0);
		zassert_equal(peer.response_ret, CONFIG_BT_L2CAP_BR_RET_TIMEOUT);
		zassert_equal(peer.response_monitor, CONFIG_BT_L2CAP_BR_MONITOR_TIMEOUT);
		wait_channel();
		zassert_equal(fixture.channel.tx.ret_timeout, CONFIG_BT_L2CAP_BR_RET_TIMEOUT);
		zassert_equal(fixture.channel.tx.monitor_timeout,
			      CONFIG_BT_L2CAP_BR_MONITOR_TIMEOUT);
		close_link();
	}
}

ZTEST(l2cap_br_timeouts, test_ertm_response_records_peer_without_changing_local_timers)
{
	struct net_buf *buf;
	int err;

	connect_channel(BT_L2CAP_BR_LINK_MODE_ERET, 0, 0, false);
	wait_channel();
	zassert_equal(fixture.channel.rx.ret_timeout, PEER_RET_TIMEOUT);
	zassert_equal(fixture.channel.rx.monitor_timeout, PEER_MONITOR_TIMEOUT);
	zassert_equal(fixture.channel.tx.ret_timeout, CONFIG_BT_L2CAP_BR_RET_TIMEOUT);
	zassert_equal(fixture.channel.tx.monitor_timeout, CONFIG_BT_L2CAP_BR_MONITOR_TIMEOUT);

	buf = net_buf_alloc(&data_pool, K_NO_WAIT);
	zassert_not_null(buf);
	net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);
	net_buf_add_u8(buf, 0x42);
	err = bt_l2cap_chan_send(&fixture.channel.chan, buf);
	if (err != 0) {
		net_buf_unref(buf);
	}
	zassert_ok(err);
	zassert_ok(k_sem_take(&iframe_sent, K_SECONDS(5)), "No I-frame sent");

	/* Observe actual RR Poll frames, allowing two ticks for transport/workqueue scheduling. */
	zassert_ok(k_sem_take(&poll_sent, K_MSEC(CONFIG_BT_L2CAP_BR_RET_TIMEOUT + 1000)));
	zassert_within(peer.poll_time[0] - peer.iframe_time, CONFIG_BT_L2CAP_BR_RET_TIMEOUT,
		       k_ticks_to_ms_ceil64(2));
	zassert_ok(k_sem_take(&poll_sent, K_MSEC(CONFIG_BT_L2CAP_BR_MONITOR_TIMEOUT + 1000)));
	zassert_within(peer.poll_time[1] - peer.poll_time[0], CONFIG_BT_L2CAP_BR_MONITOR_TIMEOUT,
		       k_ticks_to_ms_ceil64(2));
}

ZTEST(l2cap_br_timeouts, test_other_modes_keep_request_and_response_timeouts)
{
	const uint8_t modes[] = {
		BT_L2CAP_BR_LINK_MODE_RET,
		BT_L2CAP_BR_LINK_MODE_FC,
		BT_L2CAP_BR_LINK_MODE_STREAM,
	};

	for (size_t i = 0; i < ARRAY_SIZE(modes); i++) {
		connect_channel(modes[i], 900, 1500, false);
		wait_channel();
		zassert_equal(peer.sent_ret[0], CONFIG_BT_L2CAP_BR_RET_TIMEOUT);
		zassert_equal(peer.sent_monitor[0], CONFIG_BT_L2CAP_BR_MONITOR_TIMEOUT);
		zassert_equal(peer.response_result, 0);
		zassert_equal(peer.response_ret, 900);
		zassert_equal(peer.response_monitor, 1500);
		zassert_equal(fixture.channel.tx.ret_timeout, 900);
		zassert_equal(fixture.channel.tx.monitor_timeout, 1500);
		zassert_equal(fixture.channel.rx.ret_timeout, CONFIG_BT_L2CAP_BR_RET_TIMEOUT);
		zassert_equal(fixture.channel.rx.monitor_timeout,
			      CONFIG_BT_L2CAP_BR_MONITOR_TIMEOUT);
		close_link();
	}
}

ZTEST_SUITE(l2cap_br_timeouts, NULL, setup, NULL, after, NULL);
