/* btp_spp.c - Bluetooth SPP (Serial Port Profile) Tester */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/uuid.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME btp_spp
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#include "btp/btp.h"
#define SDP_CLIENT_BUF_LEN 512
#define SPP_RFCOMM_CHANNEL_MIN 1
#define SPP_RFCOMM_CHANNEL_MAX 31
NET_BUF_POOL_FIXED_DEFINE(sdp_pool, CONFIG_BT_MAX_CONN, SDP_CLIENT_BUF_LEN, 8, NULL);

static uint8_t spp_channel;
static struct bt_sdp_attribute spp_attrs_channel[] = {
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
				&spp_channel
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

static struct bt_sdp_record spp_rec_channel = BT_SDP_RECORD(spp_attrs_channel);
/*
 * Register SPP server SDP record only.
 * The actual RFCOMM server registration (bt_rfcomm_server_register()) is
 * triggered separately by AutoPTS after the SDP registration callback fires.
 */
static int spp_server_register(uint8_t channel)
{
	int err;

	spp_channel = channel;
	err = bt_sdp_register_service(&spp_rec_channel);
	if (err < 0) {
		return err;
	}

	LOG_DBG("SPP: server registered (channel=%u)", spp_channel);
	return 0;
}

/* ---- BTP command handlers --------------------------------------------- */
static uint8_t spp_supported(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	struct btp_spp_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_SPP, rp->data);
	*rsp_len += sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t spp_discover_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
			       const struct bt_sdp_discover_params *params)
{
	struct btp_spp_discover_ev ev;
	const bt_addr_t *peer;
	uint16_t channel;
	int err;

	if (result == NULL) {
		LOG_DBG("SDP discovery completed");
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	if (result->resp_buf == NULL) {
		LOG_ERR("SDP response buffer is NULL");
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

	/* Parse the SDP response to find RFCOMM channel */
	err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &channel);
	if (err < 0) {
		LOG_ERR("Failed to get RFCOMM channel (err %d)", err);
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

	LOG_DBG("SPP service found on RFCOMM channel %u", channel);

	/* Send discovery event to tester */
	memset(&ev, 0, sizeof(ev));

	peer = bt_conn_get_dst_br(conn);
	if (peer != NULL) {
		bt_addr_copy(&ev.address, peer);
	}
	ev.channel = (uint8_t)channel;

	tester_event(BTP_SERVICE_ID_SPP, BTP_SPP_EV_DISCOVERED, &ev, sizeof(ev));

	return BT_SDP_DISCOVER_UUID_STOP;
}

static struct bt_sdp_discover_params sdp_discover = {
	.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR,
	.func = spp_discover_cb,
	.pool = &sdp_pool,
	.uuid = BT_UUID_DECLARE_16(BT_SDP_SERIAL_PORT_SVCLASS),
};

static uint8_t spp_discover(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_spp_discover_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;
	bt_addr_t addr;

	/* Check if this is BR/EDR address type */
	if (cp->address_type != BTP_BR_ADDRESS_TYPE) {
		LOG_ERR("Invalid address type: 0x%02x", cp->address_type);
		return BTP_STATUS_FAILED;
	}

	/* Copy address */
	bt_addr_copy(&addr, &cp->address);

	/* Get BR/EDR connection */
	conn = bt_conn_lookup_addr_br(&addr);
	if (conn == NULL) {
		LOG_ERR("BR/EDR connection not found");
		return BTP_STATUS_FAILED;
	}

	/* Start SDP discovery */
	err = bt_sdp_discover(conn, &sdp_discover);
	if (err < 0) {
		LOG_ERR("SDP discovery failed (err %d)", err);
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("SPP discovery started");
	bt_conn_unref(conn);
	return BTP_STATUS_SUCCESS;
}

static uint8_t spp_register_server_cmd(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	const struct btp_spp_register_server_cmd *cp = cmd;
	int err;

	/* Validate channel number (RFCOMM channels are 1-31) */
	if (cp->channel < SPP_RFCOMM_CHANNEL_MIN || cp->channel > SPP_RFCOMM_CHANNEL_MAX) {
		LOG_ERR("Invalid RFCOMM channel: %u", cp->channel);
		return BTP_STATUS_FAILED;
	}

	/* Register SPP server with specified channel */
	err = spp_server_register(cp->channel);
	if (err < 0) {
		LOG_ERR("SPP server registration failed (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("SPP server registered on channel %u", cp->channel);
	return BTP_STATUS_SUCCESS;
}

/* ---- registration ------------------------------------------------------ */
static const struct btp_handler spp_handlers[] = {
	{
		.opcode     = BTP_SPP_READ_SUPPORTED_COMMANDS,
		.index      = BTP_INDEX_NONE,
		.expect_len = 0,
		.func       = spp_supported,
	},
	{
		.opcode     = BTP_SPP_CMD_DISCOVER,
		.expect_len = sizeof(struct btp_spp_discover_cmd),
		.func       = spp_discover,
	},
	{
		.opcode     = BTP_SPP_CMD_REGISTER_SERVER,
		.expect_len = sizeof(struct btp_spp_register_server_cmd),
		.func       = spp_register_server_cmd,
	},
};

uint8_t tester_init_spp(void)
{
	tester_register_command_handlers(BTP_SERVICE_ID_SPP, spp_handlers,
					 ARRAY_SIZE(spp_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_spp(void)
{
	return BTP_STATUS_SUCCESS;
}
