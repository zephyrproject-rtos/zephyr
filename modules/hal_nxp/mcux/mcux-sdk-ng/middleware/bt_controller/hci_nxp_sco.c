/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_nxp_sco);

#include "common/bt_str.h"

#include "host/conn_internal.h"
#include "host/classic/sco_internal.h"

#define NXP_VS_CMD(_opcode, _flags, _data...) \
	{ \
		.opcode = (_opcode), \
		.flags = (_flags), \
		.data_len = sizeof((uint8_t[]){ _data }), \
		.data = (uint8_t[]){ _data } \
	}

#define NXP_VS_CMD_FLAG_NARROWBAND   BIT(0)
#define NXP_VS_CMD_FLAG_WIDEBAND     BIT(1)
#define NXP_VS_CMD_FLAG_NORMAL_PINS  BIT(2)
#define NXP_VS_CMD_FLAG_REVERSE_PINS BIT(3)

#define NXP_VS_CMD_FLAG_BAND_NC (NXP_VS_CMD_FLAG_NARROWBAND | NXP_VS_CMD_FLAG_WIDEBAND)
#define NXP_VS_CMD_FLAG_PINS_NC (NXP_VS_CMD_FLAG_NORMAL_PINS | NXP_VS_CMD_FLAG_REVERSE_PINS)

struct bt_hci_nxp_vs_cmd {
	uint16_t opcode;
	uint8_t flags;
	uint16_t data_len;
	uint8_t *data;
};

static const struct bt_hci_nxp_vs_cmd sco_init_vs_cmds[] = {
	NXP_VS_CMD(BT_OP(BT_OGF_VS, 0x0028), NXP_VS_CMD_FLAG_NARROWBAND | NXP_VS_CMD_FLAG_PINS_NC,
		   0x03, 0x00, 0x03),
	NXP_VS_CMD(BT_OP(BT_OGF_VS, 0x0028), NXP_VS_CMD_FLAG_WIDEBAND | NXP_VS_CMD_FLAG_PINS_NC,
		   0x03, 0x00, 0x07),
	NXP_VS_CMD(BT_OP(BT_OGF_VS, 0x0007), NXP_VS_CMD_FLAG_BAND_NC | NXP_VS_CMD_FLAG_REVERSE_PINS,
		   0x03),
	NXP_VS_CMD(BT_OP(BT_OGF_VS, 0x0007), NXP_VS_CMD_FLAG_BAND_NC | NXP_VS_CMD_FLAG_NORMAL_PINS,
		   0x02),
	NXP_VS_CMD(BT_OP(BT_OGF_VS, 0x0029), NXP_VS_CMD_FLAG_BAND_NC | NXP_VS_CMD_FLAG_PINS_NC,
		   0x04, 0x00),
	NXP_VS_CMD(BT_OP(BT_OGF_VS, 0x001d), NXP_VS_CMD_FLAG_BAND_NC | NXP_VS_CMD_FLAG_PINS_NC,
		   0x01),
	NXP_VS_CMD(BT_OP(BT_OGF_VS, 0x0070), NXP_VS_CMD_FLAG_BAND_NC | NXP_VS_CMD_FLAG_PINS_NC,
		   0x01),
	NXP_VS_CMD(BT_OP(BT_OGF_VS, 0x0073), NXP_VS_CMD_FLAG_NARROWBAND | NXP_VS_CMD_FLAG_PINS_NC,
		   0x00),
	NXP_VS_CMD(BT_OP(BT_OGF_VS, 0x0073), NXP_VS_CMD_FLAG_WIDEBAND | NXP_VS_CMD_FLAG_PINS_NC,
		   0x01),
	NXP_VS_CMD(BT_OP(BT_OGF_VS, 0x0028), NXP_VS_CMD_FLAG_NARROWBAND | NXP_VS_CMD_FLAG_PINS_NC,
		   0x03, 0x00, 0x03),
	NXP_VS_CMD(BT_OP(BT_OGF_VS, 0x0028), NXP_VS_CMD_FLAG_WIDEBAND | NXP_VS_CMD_FLAG_PINS_NC,
		   0x03, 0x00, 0x07),
};

static const struct bt_hci_nxp_vs_cmd sco_start_vs_cmds[] = {
	NXP_VS_CMD(BT_OP(BT_OGF_VS, 0x006f), NXP_VS_CMD_FLAG_BAND_NC | NXP_VS_CMD_FLAG_PINS_NC,
		   0x00, 0x00, 0x08, 0x00, 0x00, 0x00),
};

static const struct bt_hci_nxp_vs_cmd sco_stop_vs_cmds[] = {
	NXP_VS_CMD(BT_OP(BT_OGF_VS, 0x0073), NXP_VS_CMD_FLAG_NARROWBAND | NXP_VS_CMD_FLAG_PINS_NC,
		   0x00),
	NXP_VS_CMD(BT_OP(BT_OGF_VS, 0x0073), NXP_VS_CMD_FLAG_WIDEBAND | NXP_VS_CMD_FLAG_PINS_NC,
		   0x01),
};

static int nxp_send_vs_cmd(const struct bt_hci_nxp_vs_cmd *cmd, uint8_t flags)
{
	int err;
	struct net_buf *buf;

	if ((cmd->flags & flags) != flags) {
		/* Skip the VS command */
		return 0;
	}

	buf = bt_hci_cmd_alloc(K_FOREVER);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	__ASSERT(net_buf_tailroom(buf) >= cmd->data_len, "No space in buffer");

	net_buf_add_mem(buf, cmd->data, cmd->data_len);
	err = bt_hci_cmd_send_sync(cmd->opcode, buf, NULL);
	if (err == -EACCES) {
		LOG_WRN("VS opcode %04x is disallowed", cmd->opcode);
		/* Ignore the disallowed command and continue */
		return 0;
	}

	if (err != 0) {
		LOG_ERR("Failed to send VS cmd");
	}

	return err;
}

static void bt_nxp_sco_init(uint16_t voice_setting)
{
	uint8_t air_coding_fmt;
	uint8_t flags;

	air_coding_fmt = BT_HCI_VOICE_SETTING_AIR_CODING_FMT_GET(voice_setting);

	switch (air_coding_fmt) {
	case BT_HCI_VOICE_SETTING_AIR_CODING_FMT_CVSD:
		flags = NXP_VS_CMD_FLAG_NARROWBAND;
		break;
	case BT_HCI_VOICE_SETTING_AIR_CODING_FMT_TRANSPARENT:
		flags = NXP_VS_CMD_FLAG_WIDEBAND;
		break;
	default:
		LOG_ERR("Unsupported air coding format %u", air_coding_fmt);
		return;
	}

	flags |= IS_ENABLED(CONFIG_BT_NXP_PCM_PINS_DIR_REVERSE) ? NXP_VS_CMD_FLAG_REVERSE_PINS
								: NXP_VS_CMD_FLAG_NORMAL_PINS;

	ARRAY_FOR_EACH(sco_init_vs_cmds, i) {
		if (nxp_send_vs_cmd(&sco_init_vs_cmds[i], flags) != 0) {
			LOG_ERR("Failed to send VS cmd %u", i);
			return;
		}
	}
}

static void bt_nxp_setup_sco_cmd(struct bt_conn *acl_conn, struct bt_hci_cp_setup_sync_conn *cp)
{
	uint16_t voice_setting;

	voice_setting = sys_le16_to_cpu(cp->content_format);

	LOG_DBG("Setup SCO with voice setting %04x", voice_setting);
	bt_nxp_sco_init(voice_setting);
}

static void bt_nxp_accept_sco_req_cmd(struct bt_hci_cp_accept_sync_conn_req *cp)
{
	uint16_t voice_setting;

	voice_setting = sys_le16_to_cpu(cp->content_format);
	LOG_DBG("Accept SCO req with voice setting %04x", voice_setting);
	bt_nxp_sco_init(voice_setting);
}

BT_SCO_HCI_CB_DEFINE(hci_nxp_sco_hci_cbs) = {
	.setup_sco_cmd = bt_nxp_setup_sco_cmd,
	.accept_sco_req_cmd = bt_nxp_accept_sco_req_cmd,
};

void bt_nxp_sco_connected(struct bt_conn *conn, uint8_t err)
{
	uint8_t flags;
	uint8_t air_mode;

	if (err != BT_HCI_ERR_SUCCESS) {
		return;
	}

	air_mode = conn->sco.air_mode;
	switch (air_mode) {
	case BT_HCI_CODING_FORMAT_CVSD:
		flags = NXP_VS_CMD_FLAG_NARROWBAND;
		break;
	case BT_HCI_CODING_FORMAT_TRANSPARENT:
		flags = NXP_VS_CMD_FLAG_WIDEBAND;
		break;
	default:
		LOG_ERR("Unsupported air mode %u", air_mode);
		return;
	}

	ARRAY_FOR_EACH(sco_start_vs_cmds, i) {
		if (nxp_send_vs_cmd(&sco_start_vs_cmds[i], flags) != 0) {
			LOG_ERR("Failed to send VS cmd %u", i);
			return;
		}
	}
}

void bt_nxp_sco_disconnected(struct bt_conn *conn, uint8_t reason)
{
	uint8_t flags;
	uint8_t air_mode;

	air_mode = conn->sco.air_mode;
	switch (air_mode) {
	case BT_HCI_CODING_FORMAT_CVSD:
		flags = NXP_VS_CMD_FLAG_NARROWBAND;
		break;
	case BT_HCI_CODING_FORMAT_TRANSPARENT:
		flags = NXP_VS_CMD_FLAG_WIDEBAND;
		break;
	default:
		LOG_ERR("Unsupported air mode %u", air_mode);
		return;
	}

	ARRAY_FOR_EACH(sco_stop_vs_cmds, i) {
		if (nxp_send_vs_cmd(&sco_stop_vs_cmds[i], flags) != 0) {
			LOG_ERR("Failed to send VS cmd %u", i);
			return;
		}
	}
}

BT_SCO_CONN_CB_DEFINE(hci_nxp_sco_conn_cb) = {
	.connected = bt_nxp_sco_connected,
	.disconnected = bt_nxp_sco_disconnected,
};
