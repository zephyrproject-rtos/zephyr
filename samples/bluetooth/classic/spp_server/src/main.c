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
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/settings/settings.h>

LOG_MODULE_REGISTER(spp_server, LOG_LEVEL_INF);

#define RFCOMM_MTU CONFIG_BT_RFCOMM_L2CAP_MTU

NET_BUF_POOL_FIXED_DEFINE(tx_pool, 1, BT_RFCOMM_BUF_SIZE(RFCOMM_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static struct bt_rfcomm_server rfcomm_server;
static struct bt_rfcomm_dlc rfcomm_dlc;

static struct bt_sdp_attribute spp_sdp_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
				&rfcomm_server.channel
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0102)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("Serial Port"),
};

static struct bt_sdp_record spp_sdp_rec = BT_SDP_RECORD(spp_sdp_attrs);

static void rfcomm_connected(struct bt_rfcomm_dlc *dlc)
{
	ARG_UNUSED(dlc);

	LOG_INF("RFCOMM connected (channel %u)", rfcomm_server.channel);
}

static void rfcomm_disconnected(struct bt_rfcomm_dlc *dlc)
{
	ARG_UNUSED(dlc);

	LOG_INF("RFCOMM disconnected");
}

static void rfcomm_recv(struct bt_rfcomm_dlc *dlc, struct net_buf *buf)
{
	struct net_buf *tx_buf;
	int err;

	LOG_HEXDUMP_INF(buf->data, buf->len, "RX");

	tx_buf = bt_rfcomm_create_pdu(&tx_pool);
	if (tx_buf == NULL) {
		LOG_ERR("Failed to allocate TX buffer");
		return;
	}

	if (net_buf_tailroom(tx_buf) < buf->len) {
		LOG_ERR("TX buffer too small");
		net_buf_unref(tx_buf);
		return;
	}

	net_buf_add_mem(tx_buf, buf->data, buf->len);

	err = bt_rfcomm_dlc_send(dlc, tx_buf);
	if (err < 0) {
		LOG_ERR("Echo send failed (err %d)", err);
		net_buf_unref(tx_buf);
	}
}

static struct bt_rfcomm_dlc_ops rfcomm_ops = {
	.connected = rfcomm_connected,
	.disconnected = rfcomm_disconnected,
	.recv = rfcomm_recv,
};

static int rfcomm_accept(struct bt_conn *conn, struct bt_rfcomm_server *server,
			 struct bt_rfcomm_dlc **dlc)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(server);

	if (rfcomm_dlc.session != NULL) {
		return -ENOMEM;
	}

	rfcomm_dlc.ops = &rfcomm_ops;
	rfcomm_dlc.mtu = RFCOMM_MTU;
	rfcomm_dlc.required_sec_level = BT_SECURITY_L2;
	*dlc = &rfcomm_dlc;

	return 0;
}

static void bt_ready(int err)
{
	if (err != 0) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	rfcomm_server.accept = rfcomm_accept;

	err = bt_rfcomm_server_register(&rfcomm_server);
	if (err != 0) {
		LOG_ERR("RFCOMM server register failed (err %d)", err);
		return;
	}

	err = bt_sdp_register_service(&spp_sdp_rec);
	if (err != 0) {
		LOG_ERR("SDP service register failed (err %d)", err);
		bt_rfcomm_server_unregister(&rfcomm_server);
		return;
	}

	LOG_INF("Serial Port service registered on RFCOMM channel %u", rfcomm_server.channel);

	err = bt_br_set_connectable(true, NULL);
	if (err != 0) {
		LOG_ERR("Set connectable failed (err %d)", err);
		return;
	}

	err = bt_br_set_discoverable(true, false);
	if (err != 0) {
		LOG_ERR("Set discoverable failed (err %d)", err);
		return;
	}

	LOG_INF("Ready for RFCOMM connections");
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
