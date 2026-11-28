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
#define SPP_SERVER_NAME "spp_server"
#define SPP_CLIENT_TX_MSG "Hello from SPP client"

NET_BUF_POOL_FIXED_DEFINE(tx_pool, 1, BT_RFCOMM_BUF_SIZE(RFCOMM_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
NET_BUF_POOL_FIXED_DEFINE(sdp_pool, 1, SDP_CLIENT_BUF_LEN, 8, NULL);

static struct bt_rfcomm_dlc rfcomm_dlc;
static struct bt_conn *default_conn;
static struct bt_br_discovery_param br_discover;
static struct bt_br_discovery_result scan_result[DISCOVER_RESULT_COUNT];
static struct k_work_delayable send_work;
static bool rfcomm_connected;

static struct bt_sdp_discover_params sdp_discover = {
	.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR,
	.pool = &sdp_pool,
	.uuid = BT_UUID_DECLARE_16(BT_SDP_SERIAL_PORT_SVCLASS),
};

static void rfcomm_send_sample(struct k_work *work)
{
	static const char msg[] = SPP_CLIENT_TX_MSG;
	struct net_buf *buf;
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

	if (net_buf_tailroom(buf) < sizeof(msg) - 1U) {
		LOG_ERR("TX buffer too small");
		net_buf_unref(buf);
		return;
	}

	net_buf_add_mem(buf, msg, sizeof(msg) - 1U);
	LOG_INF("TX: \"%s\" (%u bytes)", msg, (unsigned int)(sizeof(msg) - 1U));

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

	LOG_INF("RX: \"%.*s\" (%u bytes)", (int)buf->len, buf->data, buf->len);
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
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	err = bt_sdp_get_proto_param(response->resp_buf, BT_SDP_PROTO_RFCOMM, &channel);
	if (err < 0) {
		LOG_ERR("Failed to get RFCOMM channel from SDP (err %d)", err);
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	LOG_INF("SDP found RFCOMM channel %u", channel);

	err = rfcomm_connect(conn, channel);
	if (err < 0) {
		LOG_ERR("RFCOMM connect failed after SDP (err %d)", err);
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}

	return BT_SDP_DISCOVER_UUID_STOP;
}

static void start_rfcomm_connect(struct bt_conn *conn)
{
	int err;

	sdp_discover.func = sdp_discover_cb;

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
	int err;

	bt_addr_to_str(bt_conn_get_dst_br(conn), addr, sizeof(addr));
	LOG_INF("ACL link down from %s (reason 0x%02x)", addr, reason);

	if (default_conn == conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}

	err = bt_br_discovery_start(&br_discover, scan_result, DISCOVER_RESULT_COUNT);
	if (err != 0) {
		LOG_ERR("Discovery restart failed (err %d)", err);
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static bool eir_name_matches(const uint8_t eir[BT_BR_EIR_SIZE_MAX], const char *name)
{
	const uint8_t *data = eir;
	size_t remaining = BT_BR_EIR_SIZE_MAX;
	size_t name_len = strlen(name);

	while ((remaining > 2U) && (data[0] > 1U) && (data[0] < remaining)) {
		uint8_t len = data[0];
		uint8_t type = data[1];

		if ((type == BT_DATA_NAME_SHORTENED || type == BT_DATA_NAME_COMPLETE) &&
		    (len - 1U) == name_len &&
		    memcmp(&data[2], name, name_len) == 0) {
			return true;
		}

		remaining -= (size_t)len + 1U;
		data += len + 1U;
	}

	return false;
}

static void discovery_timeout_cb(const struct bt_br_discovery_result *results, size_t count)
{
	const struct bt_br_discovery_result *best = NULL;
	const struct bt_br_discovery_result *named = NULL;
	int err;

	if (count == 0) {
		LOG_INF("No devices found, restarting discovery");
		err = bt_br_discovery_start(&br_discover, scan_result, DISCOVER_RESULT_COUNT);
		if (err != 0) {
			LOG_ERR("Discovery restart failed (err %d)", err);
		}
		return;
	}

	for (size_t i = 0; i < count; i++) {
		bool name_match = eir_name_matches(results[i].eir, SPP_SERVER_NAME);

		LOG_INF("Device[%zu]: %s, rssi %d%s", i, bt_addr_str(&results[i].addr),
			results[i].rssi, name_match ? ", name spp_server" : "");

		if (name_match &&
		    (named == NULL || results[i].rssi > named->rssi)) {
			named = &results[i];
		}

		if (best == NULL || results[i].rssi > best->rssi) {
			best = &results[i];
		}
	}

	if (named != NULL) {
		best = named;
		LOG_INF("Connecting to %s (%s, rssi %d)", SPP_SERVER_NAME,
			bt_addr_str(&best->addr), best->rssi);
	} else {
		LOG_INF("Connecting to strongest RSSI peer %s (rssi %d)",
			bt_addr_str(&best->addr), best->rssi);
	}

	default_conn = bt_conn_create_br(&best->addr, BT_BR_CONN_PARAM_DEFAULT);
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
