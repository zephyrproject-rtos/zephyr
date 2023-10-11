/* btp_aics.c - Bluetooth AICS Tester */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/micp.h>
#include <zephyr/bluetooth/audio/aics.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "bap_endpoint.h"
#include "btp/btp.h"

#define LOG_MODULE_NAME bttester_aics
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#define BT_AICS_MAX_INPUT_DESCRIPTION_SIZE 16
#define BT_AICS_MAX_OUTPUT_DESCRIPTION_SIZE 16

struct btp_aics_instance aics_client_instance;
struct btp_aics_instance aics_server_instance;

static struct net_buf_simple *rx_ev_buf = NET_BUF_SIMPLE(BT_AICS_MAX_INPUT_DESCRIPTION_SIZE +
							 sizeof(struct btp_aics_description_ev));

static uint8_t aics_supported_commands(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	struct btp_aics_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_AICS_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_AICS_SET_GAIN);
	tester_set_bit(rp->data, BTP_AICS_MUTE);
	tester_set_bit(rp->data, BTP_AICS_UNMUTE);
	tester_set_bit(rp->data, BTP_AICS_MAN_GAIN_SET);
	tester_set_bit(rp->data, BTP_AICS_AUTO_GAIN_SET);
	tester_set_bit(rp->data, BTP_AICS_SET_MAN_GAIN_ONLY);

	/* octet 1 */
	tester_set_bit(rp->data, BTP_AICS_SET_AUTO_GAIN_ONLY);
	tester_set_bit(rp->data, BTP_AICS_AUDIO_DESCRIPTION_SET);
	tester_set_bit(rp->data, BTP_AICS_MUTE_DISABLE);
	tester_set_bit(rp->data, BTP_AICS_GAIN_SETTING_PROP_GET);
	tester_set_bit(rp->data, BTP_AICS_TYPE_GET);
	tester_set_bit(rp->data, BTP_AICS_STATUS_GET);
	tester_set_bit(rp->data, BTP_AICS_STATE_GET);

	/* octet 2 */
	tester_set_bit(rp->data, BTP_AICS_DESCRIPTION_GET);

	*rsp_len = sizeof(*rp) + 2;

	return BTP_STATUS_SUCCESS;
}

void btp_send_aics_state_ev(struct bt_conn *conn, uint8_t att_status, int8_t gain, uint8_t mute,
			    uint8_t mode)
{
	struct btp_aics_state_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.att_status = att_status;
	ev.gain = gain;
	ev.mute = mute;
	ev.mode = mode;

	tester_event(BTP_SERVICE_ID_AICS, BTP_AICS_STATE_EV, &ev, sizeof(ev));
}

void btp_send_gain_setting_properties_ev(struct bt_conn *conn,  uint8_t att_status, uint8_t units,
					 int8_t minimum, int8_t maximum)
{
	struct btp_gain_setting_properties_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.att_status = att_status;
	ev.units = units;
	ev.minimum = minimum;
	ev.maximum = maximum;

	tester_event(BTP_SERVICE_ID_AICS, BTP_GAIN_SETTING_PROPERTIES_EV, &ev, sizeof(ev));
}

void btp_send_aics_input_type_event(struct bt_conn *conn, uint8_t att_status, uint8_t input_type)
{
	struct btp_aics_input_type_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.att_status = att_status;
	ev.input_type = input_type;

	tester_event(BTP_SERVICE_ID_AICS, BTP_AICS_INPUT_TYPE_EV, &ev, sizeof(ev));
}

void btp_send_aics_status_ev(struct bt_conn *conn, uint8_t att_status, bool active)
{
	struct btp_aics_status_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.att_status = att_status;
	ev.active = active;

	tester_event(BTP_SERVICE_ID_AICS, BTP_AICS_STATUS_EV, &ev, sizeof(ev));
}

void btp_send_aics_description_ev(struct bt_conn *conn, uint8_t att_status, uint8_t data_len,
				  char *description)
{
	struct btp_aics_description_ev *ev;

	net_buf_simple_init(rx_ev_buf, 0);

	ev = net_buf_simple_add(rx_ev_buf, sizeof(*ev));

	bt_addr_le_copy(&ev->address, bt_conn_get_dst(conn));

	ev->att_status = att_status;
	ev->data_len = data_len;
	memcpy(ev->data, description, data_len);

	tester_event(BTP_SERVICE_ID_AICS, BTP_AICS_DESCRIPTION_EV, ev, sizeof(*ev) + data_len);
}

void btp_send_aics_procedure_ev(struct bt_conn *conn, uint8_t att_status, uint8_t opcode)
{
	struct btp_aics_procedure_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.att_status = att_status;
	ev.opcode = opcode;

	tester_event(BTP_SERVICE_ID_AICS, BTP_AICS_PROCEDURE_EV, &ev, sizeof(ev));
}

static uint8_t aics_set_gain(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_aics_set_gain_cmd *cp = cmd;

	LOG_DBG("AICS set gain %d", cp->gain);

	if (!bt_addr_le_eq(&cp->address, BT_ADDR_LE_ANY)) {
		if (bt_aics_gain_set(aics_client_instance.aics[0], cp->gain) != 0) {
			return BTP_STATUS_FAILED;
		}
	} else {
		for (uint8_t i = 0; i < aics_server_instance.aics_cnt; i++) {
			if (bt_aics_gain_set(aics_server_instance.aics[i], cp->gain) != 0) {
				return BTP_STATUS_FAILED;
			}
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t aics_unmute(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_aics_unmute_cmd *cp = cmd;

	LOG_DBG("AICS Unmute");

	if (!bt_addr_le_eq(&cp->address, BT_ADDR_LE_ANY)) {
		if (bt_aics_unmute(aics_client_instance.aics[0]) != 0) {
			return BTP_STATUS_FAILED;
		}
	} else {
		for (uint8_t i = 0; i < aics_server_instance.aics_cnt; i++) {
			if (bt_aics_unmute(aics_server_instance.aics[i]) != 0) {
				return BTP_STATUS_FAILED;
			}
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t aics_mute(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_aics_mute_cmd *cp = cmd;

	LOG_DBG("AICS Mute");

	if (!bt_addr_le_eq(&cp->address, BT_ADDR_LE_ANY)) {
		if (bt_aics_mute(aics_client_instance.aics[0]) != 0) {
			return BTP_STATUS_FAILED;
		}
	} else {
		for (uint8_t i = 0; i < aics_server_instance.aics_cnt; i++) {
			if (bt_aics_mute(aics_server_instance.aics[i]) != 0) {
				return BTP_STATUS_FAILED;
			}
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t aics_state_get(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_aics_state_cmd *cp = cmd;

	LOG_DBG("AICS State");

	if (!bt_addr_le_eq(&cp->address, BT_ADDR_LE_ANY)) {
		if (bt_aics_state_get(aics_client_instance.aics[0]) != 0) {
			return BTP_STATUS_FAILED;
		}
	} else {
		for (uint8_t i = 0; i < aics_server_instance.aics_cnt; i++) {
			if (bt_aics_state_get(aics_server_instance.aics[i]) != 0) {
				return BTP_STATUS_FAILED;
			}
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t aics_type_get(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_aics_type_cmd *cp = cmd;

	LOG_DBG("AICS Type");

	if (!bt_addr_le_eq(&cp->address, BT_ADDR_LE_ANY)) {
		if (bt_aics_type_get(aics_client_instance.aics[0]) != 0) {
			return BTP_STATUS_FAILED;
		}
	} else {
		for (uint8_t i = 0; i < aics_server_instance.aics_cnt; i++) {
			if (bt_aics_type_get(aics_server_instance.aics[i]) != 0) {
				return BTP_STATUS_FAILED;
			}
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t aics_status_get(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_aics_status_cmd *cp = cmd;

	LOG_DBG("AICS Status");

	if (!bt_addr_le_eq(&cp->address, BT_ADDR_LE_ANY)) {
		if (bt_aics_status_get(aics_client_instance.aics[0]) != 0) {
			return BTP_STATUS_FAILED;
		}
	} else {
		for (uint8_t i = 0; i < aics_server_instance.aics_cnt; i++) {
			if (bt_aics_status_get(aics_server_instance.aics[i]) != 0) {
				return BTP_STATUS_FAILED;
			}
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t aics_gain_setting_prop_get(const void *cmd, uint16_t cmd_len, void *rsp,
					  uint16_t *rsp_len)
{
	const struct btp_aics_gain_setting_prop_cmd *cp = cmd;

	LOG_DBG("AICS Gain settings properties");

	if (!bt_addr_le_eq(&cp->address, BT_ADDR_LE_ANY)) {
		if (bt_aics_gain_setting_get(aics_client_instance.aics[0]) != 0) {
			return BTP_STATUS_FAILED;
		}
	} else {
		for (uint8_t i = 0; i < aics_server_instance.aics_cnt; i++) {
			if (bt_aics_gain_setting_get(aics_server_instance.aics[i]) != 0) {
				return BTP_STATUS_FAILED;
			}
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t aics_man_gain_set(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_aics_manual_gain_cmd *cp = cmd;

	LOG_DBG("AICS set manual gain mode");

	if (!bt_addr_le_eq(&cp->address, BT_ADDR_LE_ANY)) {
		if (bt_aics_manual_gain_set(aics_client_instance.aics[0]) != 0) {
			return BTP_STATUS_FAILED;
		}
	} else {
		for (uint8_t i = 0; i < aics_server_instance.aics_cnt; i++) {
			if (bt_aics_manual_gain_set(aics_server_instance.aics[i]) != 0) {
				return BTP_STATUS_FAILED;
			}
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t aics_auto_gain_set(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_aics_auto_gain_cmd *cp = cmd;

	LOG_DBG("AICS set automatic gain mode");

	if (!bt_addr_le_eq(&cp->address, BT_ADDR_LE_ANY)) {
		if (bt_aics_automatic_gain_set(aics_client_instance.aics[0]) != 0) {
			return BTP_STATUS_FAILED;
		}
	} else {
		for (uint8_t i = 0; i < aics_server_instance.aics_cnt; i++) {
			if (bt_aics_automatic_gain_set(aics_server_instance.aics[i]) != 0) {
				return BTP_STATUS_FAILED;
			}
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t aics_set_man_gain_only(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	LOG_DBG("AICS manual gain only set");

	for (uint8_t i = 0; i < aics_server_instance.aics_cnt; i++) {
		if (bt_aics_gain_set_manual_only(aics_server_instance.aics[i]) != 0) {
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t aics_set_auto_gain_only(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	LOG_DBG("AICS auto gain only set");

	for (uint8_t i = 0; i < aics_server_instance.aics_cnt; i++) {
		if (bt_aics_gain_set_auto_only(aics_server_instance.aics[i]) != 0) {
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t aics_mute_disable(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	LOG_DBG("AICS disable mute");

	for (uint8_t i = 0; i < aics_server_instance.aics_cnt; i++) {
		if (bt_aics_disable_mute(aics_server_instance.aics[i]) != 0) {
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t aics_desc_set(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_aics_audio_desc_cmd *cp = cmd;
	char description[BT_AICS_MAX_INPUT_DESCRIPTION_SIZE];

	LOG_DBG("AICS set description");

	if (cmd_len < sizeof(*cp) || cmd_len != sizeof(*cp) + cp->desc_len) {
		return BTP_STATUS_FAILED;
	}

	if (cp->desc_len >= sizeof(description)) {
		return BTP_STATUS_FAILED;
	}

	if (cp->desc_len > (BT_AICS_MAX_INPUT_DESCRIPTION_SIZE - 1)) {
		return BTP_STATUS_FAILED;
	}

	memcpy(description, cp->desc, cp->desc_len);
	description[cp->desc_len] = '\0';

	for (uint8_t i = 0; i < aics_server_instance.aics_cnt; i++) {
		if (bt_aics_description_set(aics_server_instance.aics[i], description) != 0) {
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t aics_desc_get(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_aics_desc_cmd *cp = cmd;

	LOG_DBG("AICS Description");

	if (!bt_addr_le_eq(&cp->address, BT_ADDR_LE_ANY)) {
		if (bt_aics_description_get(aics_client_instance.aics[0]) != 0) {
			return BTP_STATUS_FAILED;
		}
	} else {
		for (uint8_t i = 0; i < aics_server_instance.aics_cnt; i++) {
			if (bt_aics_description_get(aics_server_instance.aics[i]) != 0) {
				return BTP_STATUS_FAILED;
			}
		}
	}

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler aics_handlers[] = {
	{
		.opcode = BTP_AICS_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = aics_supported_commands,
	},
	{
		.opcode = BTP_AICS_SET_GAIN,
		.expect_len = sizeof(struct btp_aics_set_gain_cmd),
		.func = aics_set_gain,
	},
	{
		.opcode = BTP_AICS_MUTE,
		.expect_len = sizeof(struct btp_aics_mute_cmd),
		.func = aics_mute,
	},
	{
		.opcode = BTP_AICS_UNMUTE,
		.expect_len = sizeof(struct btp_aics_unmute_cmd),
		.func = aics_unmute,
	},
	{
		.opcode = BTP_AICS_GAIN_SETTING_PROP_GET,
		.expect_len = sizeof(struct btp_aics_gain_setting_prop_cmd),
		.func = aics_gain_setting_prop_get,
	},
	{
		.opcode = BTP_AICS_MUTE_DISABLE,
		.expect_len = 0,
		.func = aics_mute_disable,
	},
	{
		.opcode = BTP_AICS_MAN_GAIN_SET,
		.expect_len = sizeof(struct btp_aics_manual_gain_cmd),
		.func = aics_man_gain_set,
	},
	{
		.opcode = BTP_AICS_AUTO_GAIN_SET,
		.expect_len = sizeof(struct btp_aics_auto_gain_cmd),
		.func = aics_auto_gain_set,
	},
	{
		.opcode = BTP_AICS_SET_AUTO_GAIN_ONLY,
		.expect_len = 0,
		.func = aics_set_auto_gain_only,
	},
	{
		.opcode = BTP_AICS_SET_MAN_GAIN_ONLY,
		.expect_len = 0,
		.func = aics_set_man_gain_only,
	},
	{
		.opcode = BTP_AICS_AUDIO_DESCRIPTION_SET,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = aics_desc_set,
	},
	{
		.opcode = BTP_AICS_DESCRIPTION_GET,
		.expect_len = sizeof(struct btp_aics_desc_cmd),
		.func = aics_desc_get,
	},
	{
		.opcode = BTP_AICS_TYPE_GET,
		.expect_len = sizeof(struct btp_aics_type_cmd),
		.func = aics_type_get,
	},
	{
		.opcode = BTP_AICS_STATUS_GET,
		.expect_len = sizeof(struct btp_aics_status_cmd),
		.func = aics_status_get,
	},
	{
		.opcode = BTP_AICS_STATE_GET,
		.expect_len = sizeof(struct btp_aics_state_cmd),
		.func = aics_state_get,
	},
};

static void aics_state_cb(struct bt_aics *inst, int err, int8_t gain, uint8_t mute, uint8_t mode)
{
	struct bt_conn *conn;

	bt_aics_client_conn_get(inst, &conn);

	if (err) {
		if (err < 0) {
			err = BT_ATT_ERR_UNLIKELY;
		}
		btp_send_aics_state_ev(conn, err, 0, 0, 0);
	} else {
		btp_send_aics_state_ev(conn, 0, gain, mute, mode);
	}

	LOG_DBG("AICS state callback (%d)", err);
}

static void aics_gain_setting_cb(struct bt_aics *inst, int err, uint8_t units, int8_t minimum,
				 int8_t maximum)
{
	struct bt_conn *conn;

	bt_aics_client_conn_get(inst, &conn);
	btp_send_gain_setting_properties_ev(conn, err, units, minimum, maximum);

	LOG_DBG("AICS gain setting callback (%d)", err);
}

static void aics_input_type_cb(struct bt_aics *inst, int err, uint8_t input_type)
{
	struct bt_conn *conn;

	bt_aics_client_conn_get(inst, &conn);
	btp_send_aics_input_type_event(conn, err, input_type);

	LOG_DBG("AICS input type callback (%d)", err);
}

static void aics_status_cb(struct bt_aics *inst, int err, bool active)
{
	struct bt_conn *conn;

	bt_aics_client_conn_get(inst, &conn);
	btp_send_aics_status_ev(conn, err, active);

	LOG_DBG("AICS status callback (%d)", err);
}

static void aics_description_cb(struct bt_aics *inst, int err, char *description)
{
	struct bt_conn *conn;
	uint8_t data_len = strlen(description);

	bt_aics_client_conn_get(inst, &conn);
	btp_send_aics_description_ev(conn, err, data_len, description);

	LOG_DBG("AICS description callback (%d)", err);
}

static void aics_set_gain_cb(struct bt_aics *inst, int err)
{
	struct bt_conn *conn;

	bt_aics_client_conn_get(inst, &conn);

	btp_send_aics_procedure_ev(conn, err, BTP_AICS_SET_GAIN);

	LOG_DBG("AICS set gain cb (%d)", err);
}

static void aics_mute_cb(struct bt_aics *inst, int err)
{
	struct bt_conn *conn;

	bt_aics_client_conn_get(inst, &conn);

	btp_send_aics_procedure_ev(conn, err, BTP_AICS_MUTE);

	LOG_DBG("AICS mute cb (%d)", err);
}

static void aics_unmute_cb(struct bt_aics *inst, int err)
{
	struct bt_conn *conn;

	bt_aics_client_conn_get(inst, &conn);

	btp_send_aics_procedure_ev(conn, err, BTP_AICS_UNMUTE);

	LOG_DBG("AICS unmute cb (%d)", err);
}

static void aics_set_man_gain_cb(struct bt_aics *inst, int err)
{
	struct bt_conn *conn;

	bt_aics_client_conn_get(inst, &conn);

	btp_send_aics_procedure_ev(conn, err, BTP_AICS_MAN_GAIN_SET);

	LOG_DBG("AICS set manual gain cb (%d)", err);
}

static void aics_set_auto_gain_cb(struct bt_aics *inst, int err)
{
	struct bt_conn *conn;

	bt_aics_client_conn_get(inst, &conn);

	btp_send_aics_procedure_ev(conn, err, BTP_AICS_AUTO_GAIN_SET);

	LOG_DBG("AICS set automatic gain cb (%d)", err);
}

struct bt_aics_cb aics_client_cb = {
	.state = aics_state_cb,
	.gain_setting = aics_gain_setting_cb,
	.type = aics_input_type_cb,
	.status = aics_status_cb,
	.description = aics_description_cb,
#if defined(CONFIG_BT_AICS_CLIENT)
	.set_gain = aics_set_gain_cb,
	.unmute = aics_unmute_cb,
	.mute = aics_mute_cb,
	.set_manual_mode = aics_set_man_gain_cb,
	.set_auto_mode = aics_set_auto_gain_cb
#endif /* CONFIG_BT_AICS_CLIENT */
};

uint8_t tester_init_aics(void)
{
	tester_register_command_handlers(BTP_SERVICE_ID_AICS, aics_handlers,
					 ARRAY_SIZE(aics_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_aics(void)
{
	return BTP_STATUS_SUCCESS;
}
