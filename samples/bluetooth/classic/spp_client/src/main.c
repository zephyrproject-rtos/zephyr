/*
 * Copyright 2025 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/settings/settings.h>

LOG_MODULE_REGISTER(spp_client, LOG_LEVEL_INF);

#define RFCOMM_MTU CONFIG_BT_RFCOMM_L2CAP_MTU
#define SDP_CLIENT_BUF_LEN 512
#define DISCOVER_RESULT_COUNT 10

NET_BUF_POOL_FIXED_DEFINE(tx_pool, 1, BT_RFCOMM_BUF_SIZE(RFCOMM_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
NET_BUF_POOL_FIXED_DEFINE(sdp_pool, 1, SDP_CLIENT_BUF_LEN, 8, NULL);

static struct bt_rfcomm_dlc rfcomm_dlc;
static struct bt_sdp_discover_params sdp_discover;
static struct bt_conn *default_conn;
static struct bt_br_discovery_param br_discover;
static struct bt_br_discovery_result scan_result[DISCOVER_RESULT_COUNT];
static struct k_work_delayable send_work;
static bool rfcomm_connected;

static void rfcomm_send_sample(struct k_work *work)
{
	struct net_buf *buf;
	uint8_t *data;
	int err;

	ARG_UNUSED(work);

	if (!rfcomm_connected) {
		return;
	}

	buf = bt_rfcomm_create_pdu(&tx_pool);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate TX buffer");
		return;
	}

	data = net_buf_add(buf, 4);
	data[0] = 'S';
	data[1] = 'P';
	data[2] = 'P';
	data[3] = '!';

	err = bt_rfcomm_dlc_send(&rfcomm_dlc, buf);
	if (err < 0) {
		LOG_ERR("RFCOMM send failed (err %d)", err);
		net_buf_unref(buf);
	}

	(void)k_work_reschedule(&send_work, K_SECONDS(2));
}

static void rfcomm_connected_cb(struct bt_rfcomm_dlc *dlc)
{
	LOG_INF("RFCOMM connected (channel %u)", dlc->dlci >> 1);

	rfcomm_connected = true;
	(void)k_work_reschedule(&send_work, K_SECONDS(1));
}

static void rfcomm_disconnected_cb(struct bt_rfcomm_dlc *dlc)
{
	ARG_UNUSED(dlc);

	LOG_INF("RFCOMM disconnected");
	rfcomm_connected = false;
	k_work_cancel_delayable(&send_work);
}

static void rfcomm_recv(struct bt_rfcomm_dlc *dlc, struct net_buf *buf)
{
	ARG_UNUSED(dlc);

	LOG_HEXDUMP_INF(buf->data, buf->len, "RX");
}

static struct bt_rfcomm_dlc_ops rfcomm_ops = {
	.connected = rfcomm_connected_cb,
	.disconnected = rfcomm_disconnected_cb,
	.recv = rfcomm_recv,
};

static int rfcomm_connect(struct bt_conn *conn, uint8_t channel)
{
	int err;

	if (rfcomm_dlc.session != NULL) {
		return -EALREADY;
	}

	rfcomm_dlc.ops = &rfcomm_ops;
	rfcomm_dlc.mtu = RFCOMM_MTU;
	rfcomm_dlc.required_sec_level = BT_SECURITY_L2;

	err = bt_rfcomm_dlc_connect(conn, &rfcomm_dlc, channel);
	if (err != 0) {
		LOG_ERR("RFCOMM connect failed (channel %u, err %d)", channel, err);
	}

	return err;
}

static uint8_t sdp_discover_cb(struct bt_conn *conn, struct bt_sdp_client_result *response,
			       const struct bt_sdp_discover_params *params)
{
	uint16_t channel;
	int err;

	ARG_UNUSED(params);

	if (response == NULL || response->resp_buf == NULL) {
		LOG_ERR("Serial Port service not found");
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	err = bt_sdp_get_proto_param(response->resp_buf, BT_SDP_PROTO_RFCOMM, &channel);
	if (err < 0) {
		LOG_ERR("Failed to get RFCOMM channel from SDP (err %d)", err);
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	LOG_INF("SDP found RFCOMM channel %u", channel);

	err = rfcomm_connect(conn, channel);
	if (err < 0) {
		LOG_ERR("RFCOMM connect failed after SDP (err %d)", err);
	}

	return BT_SDP_DISCOVER_UUID_STOP;
}

static void start_rfcomm_connect(struct bt_conn *conn)
{
	int err;

	sdp_discover.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;
	sdp_discover.func = sdp_discover_cb;
	sdp_discover.pool = &sdp_pool;
	sdp_discover.uuid = BT_UUID_DECLARE_16(BT_SDP_SERIAL_PORT_SVCLASS);

	err = bt_sdp_discover(conn, &sdp_discover);
	if (err != 0) {
		LOG_ERR("SDP discover failed (err %d)", err);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_STR_LEN];

	bt_addr_to_str(bt_conn_get_dst_br(conn), addr, sizeof(addr));

	if (err != 0) {
		LOG_ERR("ACL connect failed (%s, err %u)", addr, err);
		return;
	}

	LOG_INF("ACL link up to %s", addr);
	default_conn = bt_conn_ref(conn);
	start_rfcomm_connect(conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_STR_LEN];

	bt_addr_to_str(bt_conn_get_dst_br(conn), addr, sizeof(addr));
	LOG_INF("ACL link down from %s (reason 0x%02x)", addr, reason);

	if (default_conn == conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void discovery_timeout_cb(const struct bt_br_discovery_result *results, size_t count)
{
	int err;

	if (count == 0) {
		LOG_INF("No devices found, restarting discovery");
		err = bt_br_discovery_start(&br_discover, scan_result, DISCOVER_RESULT_COUNT);
		if (err != 0) {
			LOG_ERR("Discovery restart failed (err %d)", err);
		}
		return;
	}

	LOG_INF("Connecting to %s", bt_addr_str(&results[0].addr));

	default_conn = bt_conn_create_br(&results[0].addr, BT_BR_CONN_PARAM_DEFAULT);
	if (default_conn == NULL) {
		LOG_ERR("Failed to create ACL connection");
		return;
	}

	bt_conn_unref(default_conn);
}

static struct bt_br_discovery_cb discovery_cb = {
	.timeout = discovery_timeout_cb,
};

static void bt_ready(int err)
{
	if (err != 0) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	bt_conn_cb_register(&conn_callbacks);
	bt_br_discovery_cb_register(&discovery_cb);

	k_work_init_delayable(&send_work, rfcomm_send_sample);

	br_discover.length = 10;
	br_discover.limited = false;

	err = bt_br_discovery_start(&br_discover, scan_result, DISCOVER_RESULT_COUNT);
	if (err != 0) {
		LOG_ERR("Discovery start failed (err %d)", err);
		return;
	}

	LOG_INF("Scanning for Serial Port services");
}

int main(void)
{
	int err;

	err = bt_enable(bt_ready);
	if (err != 0) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
	}

	return 0;
}
