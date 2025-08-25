/** @file
 *  @brief Audio Video Remote Control Profile shell functions.
 */

/*
 * Copyright (c) 2024 Xiaomi InC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/avrcp.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/l2cap.h>

#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

struct bt_avrcp_ct *default_ct;
struct bt_avrcp_tg *default_tg;
static bool avrcp_ct_registered;
static bool avrcp_tg_registered;
static uint8_t local_tid;
static uint8_t tg_tid;

static uint8_t get_next_tid(void)
{
	uint8_t ret = local_tid;

	local_tid++;
	local_tid &= 0x0F;

	return ret;
}

static void avrcp_ct_connected(struct bt_conn *conn, struct bt_avrcp_ct *ct)
{
	bt_shell_print("AVRCP CT connected");
	default_ct = ct;
	local_tid = 0;
}

static void avrcp_ct_disconnected(struct bt_avrcp_ct *ct)
{
	bt_shell_print("AVRCP CT disconnected");
	local_tid = 0;
	default_ct = NULL;
}

static void avrcp_get_cap_rsp(struct bt_avrcp_ct *ct, uint8_t tid,
			      const struct bt_avrcp_get_cap_rsp *rsp)
{
	uint8_t i;

	switch (rsp->cap_id) {
	case BT_AVRCP_CAP_COMPANY_ID:
		for (i = 0; i < rsp->cap_cnt; i++) {
			bt_shell_print("Remote CompanyID = 0x%06x",
				       sys_get_be24(&rsp->cap[BT_AVRCP_COMPANY_ID_SIZE * i]));
		}
		break;
	case BT_AVRCP_CAP_EVENTS_SUPPORTED:
		for (i = 0; i < rsp->cap_cnt; i++) {
			bt_shell_print("Remote supported EventID = 0x%02x", rsp->cap[i]);
		}
		break;
	}
}

static void avrcp_unit_info_rsp(struct bt_avrcp_ct *ct, uint8_t tid,
				struct bt_avrcp_unit_info_rsp *rsp)
{
	bt_shell_print("AVRCP unit info received, unit type = 0x%02x, company_id = 0x%06x",
		       rsp->unit_type, rsp->company_id);
}

static void avrcp_subunit_info_rsp(struct bt_avrcp_ct *ct, uint8_t tid,
				   struct bt_avrcp_subunit_info_rsp *rsp)
{
	int i;

	bt_shell_print("AVRCP subunit info received, subunit type = 0x%02x, extended subunit = %d",
		       rsp->subunit_type, rsp->max_subunit_id);
	for (i = 0; i < rsp->max_subunit_id; i++) {
		bt_shell_print("extended subunit id = %d, subunit type = 0x%02x",
			       rsp->extended_subunit_id[i], rsp->extended_subunit_type[i]);
	}
}

static void avrcp_passthrough_rsp(struct bt_avrcp_ct *ct, uint8_t tid, bt_avrcp_rsp_t result,
				  const struct bt_avrcp_passthrough_rsp *rsp)
{
	if (result == BT_AVRCP_RSP_ACCEPTED) {
		bt_shell_print(
			"AVRCP passthough command accepted, operation id = 0x%02x, state = %d",
			BT_AVRCP_PASSTHROUGH_GET_OPID(rsp), BT_AVRCP_PASSTHROUGH_GET_STATE(rsp));
	} else {
		bt_shell_print("AVRCP passthough command rejected, operation id = 0x%02x, state = "
			       "%d, response = %d",
			       BT_AVRCP_PASSTHROUGH_GET_OPID(rsp),
			       BT_AVRCP_PASSTHROUGH_GET_STATE(rsp), result);
	}
}

static struct bt_avrcp_ct_cb app_avrcp_ct_cb = {
	.connected = avrcp_ct_connected,
	.disconnected = avrcp_ct_disconnected,
	.get_cap_rsp = avrcp_get_cap_rsp,
	.unit_info_rsp = avrcp_unit_info_rsp,
	.subunit_info_rsp = avrcp_subunit_info_rsp,
	.passthrough_rsp = avrcp_passthrough_rsp,
};

static void avrcp_tg_connected(struct bt_conn *conn, struct bt_avrcp_tg *tg)
{
	bt_shell_print("AVRCP TG connected");
	default_tg = tg;
}

static void avrcp_tg_disconnected(struct bt_avrcp_tg *tg)
{
	bt_shell_print("AVRCP TG disconnected");
	default_tg = NULL;
}

static void avrcp_unit_info_req(struct bt_avrcp_tg *tg, uint8_t tid)
{
	bt_shell_print("AVRCP unit info request received");
	tg_tid = tid;
}

static void avrcp_subunit_info_req(struct bt_avrcp_tg *tg, uint8_t tid)
{
	bt_shell_print("AVRCP subunit info request received");
	tg_tid = tid;
}

static void avrcp_passthrough_cmd_req(struct bt_avrcp_tg *tg, uint8_t tid, bt_avrcp_opid_t opid,
				      bt_avrcp_button_state_t state, const uint8_t *data,
				      uint8_t len)
{
	const char *state_str;

	/* Convert button state to string */
	state_str = (state == BT_AVRCP_BUTTON_PRESSED) ? "PRESSED" : "RELEASED";

	bt_shell_print("AVRCP passthrough command received: opid = 0x%02x (%s), tid=0x%02x, len=%u",
		       opid, state_str, tid, len);

	if (len > 0U && data != NULL) {
		bt_shell_print("Payload:");
		for (uint8_t i = 0U; i < len; i++) {
			bt_shell_print("  [%u]: 0x%02x", i, data[i]);
		}
	}

	tg_tid = tid;
}

static struct bt_avrcp_tg_cb app_avrcp_tg_cb = {
	.connected = avrcp_tg_connected,
	.disconnected = avrcp_tg_disconnected,
	.unit_info_req = avrcp_unit_info_req,
	.subunit_info_req = avrcp_subunit_info_req,
	.passthrough_cmd_req = avrcp_passthrough_cmd_req,
};

static int register_ct_cb(const struct shell *sh)
{
	int err;

	if (avrcp_ct_registered) {
		return 0;
	}

	err = bt_avrcp_ct_register_cb(&app_avrcp_ct_cb);
	if (!err) {
		avrcp_ct_registered = true;
		shell_print(sh, "AVRCP CT callbacks registered");
	} else {
		shell_print(sh, "failed to register AVRCP CT callbacks");
	}

	return err;
}

static int cmd_register_ct_cb(const struct shell *sh, int32_t argc, char *argv[])
{
	if (avrcp_ct_registered) {
		shell_print(sh, "already registered");
		return 0;
	}

	register_ct_cb(sh);

	return 0;
}

static int register_tg_cb(const struct shell *sh)
{
	int err;

	if (avrcp_tg_registered) {
		return 0;
	}

	err = bt_avrcp_tg_register_cb(&app_avrcp_tg_cb);
	if (!err) {
		avrcp_tg_registered = true;
		shell_print(sh, "AVRCP TG callbacks registered");
	} else {
		shell_print(sh, "failed to register AVRCP TG callbacks");
	}

	return err;
}

static int cmd_register_tg_cb(const struct shell *sh, int32_t argc, char *argv[])
{
	if (avrcp_tg_registered) {
		shell_print(sh, "already registered");
		return 0;
	}

	register_tg_cb(sh);

	return 0;
}

static int cmd_connect(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;

	if (!avrcp_ct_registered && register_ct_cb(sh) != 0) {
		return -ENOEXEC;
	}

	if (!avrcp_tg_registered && register_tg_cb(sh) != 0) {
		return -ENOEXEC;
	}

	if (!default_conn) {
		shell_error(sh, "BR/EDR not connected");
		return -ENOEXEC;
	}

	err = bt_avrcp_connect(default_conn);
	if (err) {
		shell_error(sh, "fail to connect AVRCP");
	}

	return 0;
}

static int cmd_disconnect(const struct shell *sh, int32_t argc, char *argv[])
{
	if ((!avrcp_ct_registered) && (!avrcp_tg_registered)) {
		shell_error(sh, "Neither CT nor TG callbacks are registered.");
		return -ENOEXEC;
	}

	if (!default_conn) {
		shell_print(sh, "Not connected");
		return -ENOEXEC;
	}

	if ((default_ct != NULL) || (default_tg != NULL)) {
		bt_avrcp_disconnect(default_conn);
	} else {
		shell_error(sh, "AVRCP is not connected");
	}

	return 0;
}

static int cmd_get_unit_info(const struct shell *sh, int32_t argc, char *argv[])
{
	if (!avrcp_ct_registered && register_ct_cb(sh) != 0) {
		return -ENOEXEC;
	}

	if (default_ct != NULL) {
		bt_avrcp_ct_get_unit_info(default_ct, get_next_tid());
	} else {
		shell_error(sh, "AVRCP is not connected");
	}

	return 0;
}

static int cmd_send_unit_info_rsp(const struct shell *sh, int32_t argc, char *argv[])
{
	struct bt_avrcp_unit_info_rsp rsp;
	int err;

	if (!avrcp_tg_registered && register_tg_cb(sh) != 0) {
		return -ENOEXEC;
	}

	rsp.unit_type = BT_AVRCP_SUBUNIT_TYPE_PANEL;
	rsp.company_id = BT_AVRCP_COMPANY_ID_BLUETOOTH_SIG;

	if (default_tg != NULL) {
		err = bt_avrcp_tg_send_unit_info_rsp(default_tg, tg_tid, &rsp);
		if (!err) {
			shell_print(sh, "AVRCP send unit info response");
		} else {
			shell_error(sh, "Failed to send unit info response");
		}
	} else {
		shell_error(sh, "AVRCP is not connected");
	}

	return 0;
}

static int cmd_send_subunit_info_rsp(const struct shell *sh, int32_t argc, char *argv[])
{
	struct bt_avrcp_subunit_info_rsp rsp;
	int err;

	if (!avrcp_tg_registered && register_tg_cb(sh) != 0) {
		return -ENOEXEC;
	}

	/* Setup subunit info response with panel subunit */
	rsp.subunit_type = BT_AVRCP_SUBUNIT_TYPE_PANEL;
	rsp.max_subunit_id = 0U;

	if (default_tg != NULL) {
		err = bt_avrcp_tg_send_subunit_info_rsp(default_tg, tg_tid, &rsp);
		if (!err) {
			shell_print(sh, "AVRCP send subunit info response");
		} else {
			shell_error(sh, "Failed to send subunit info response");
		}
	} else {
		shell_error(sh, "AVRCP is not connected");
	}

	return 0;
}

static int cmd_send_passthrough_rsp(const struct shell *sh, int32_t argc, char *argv[])
{
	bt_avrcp_opid_t opid;
	bt_avrcp_button_state_t state;
	uint16_t vu_opid = 0;
	uint8_t is_op_vu = 0;
	struct bt_avrcp_passthrough_opvu_data payload;
	char *endptr;
	unsigned long val;
	int err;

	if (!avrcp_tg_registered && register_tg_cb(sh) != 0) {
		return -ENOEXEC;
	}

	if (default_tg == NULL) {
		shell_error(sh, "AVRCP TG is not connected");
		return -ENOEXEC;
	}

	memset(&payload, 0, sizeof(payload));

	if (!strcmp(argv[1], "op")) {
		is_op_vu = 0;
	} else if (!strcmp(argv[1], "opvu")) {
		opid = BT_AVRCP_OPID_VENDOR_UNIQUE;
		is_op_vu = 1;
	} else {
		shell_error(sh, "Invalid response: %s", argv[1]);
		return -EINVAL;
	}

	if (!strcmp(argv[2], "play")) {
		opid = BT_AVRCP_OPID_PLAY;
		vu_opid = (uint16_t)opid;
	} else if (!strcmp(argv[2], "pause")) {
		opid = BT_AVRCP_OPID_PAUSE;
		vu_opid = (uint16_t)opid;
	} else {
		/* Try to parse as hex value */
		val = strtoul(argv[2], &endptr, 16);
		if (*endptr != '\0' || val > 0xFFFFU) {
			shell_error(sh, "Invalid opid: %s", argv[2]);
			return -EINVAL;
		}
		if (is_op_vu == 1) {
			vu_opid = (uint16_t)val;
		} else {
			opid = (bt_avrcp_opid_t)val;
		}
	}

	if (!strcmp(argv[3], "pressed")) {
		state = BT_AVRCP_BUTTON_PRESSED;
	} else if (!strcmp(argv[3], "released")) {
		state = BT_AVRCP_BUTTON_RELEASED;
	} else {
		shell_error(sh, "Invalid state: %s", argv[3]);
		return -EINVAL;
	}

	if (is_op_vu == 1) {
		sys_put_be24(BT_AVRCP_COMPANY_ID_BLUETOOTH_SIG, payload.company_id);
		payload.op_len = 5U;
		payload.opid_vu = vu_opid;
	}

	err = bt_avrcp_tg_send_passthrough_rsp(default_tg, tg_tid, BT_AVRCP_RSP_ACCEPTED, opid,
					       state, payload.op_len ? (const uint8_t *)&payload
					       : NULL, payload.op_len);
	if (err) {
		shell_error(sh, "Failed to send passthrough response: %d", err);
	} else {
		shell_print(sh, "Passthrough opid=0x%02x, state=%s", opid, argv[2]);
	}

	return err;
}

static int cmd_get_subunit_info(const struct shell *sh, int32_t argc, char *argv[])
{
	if (!avrcp_ct_registered && register_ct_cb(sh) != 0) {
		return -ENOEXEC;
	}

	if (default_ct != NULL) {
		bt_avrcp_ct_get_subunit_info(default_ct, get_next_tid());
	} else {
		shell_error(sh, "AVRCP is not connected");
	}

	return 0;
}

static int cmd_passthrough(const struct shell *sh, bt_avrcp_opid_t opid, const uint8_t *payload,
			   uint8_t len)
{
	if (!avrcp_ct_registered && register_ct_cb(sh) != 0) {
		return -ENOEXEC;
	}

	if (default_ct != NULL) {
		bt_avrcp_ct_passthrough(default_ct, get_next_tid(), opid, BT_AVRCP_BUTTON_PRESSED,
					payload, len);
		bt_avrcp_ct_passthrough(default_ct, get_next_tid(), opid, BT_AVRCP_BUTTON_RELEASED,
					payload, len);
	} else {
		shell_error(sh, "AVRCP is not connected");
	}

	return 0;
}

static int cmd_play(const struct shell *sh, int32_t argc, char *argv[])
{
	return cmd_passthrough(sh, BT_AVRCP_OPID_PLAY, NULL, 0);
}

static int cmd_pause(const struct shell *sh, int32_t argc, char *argv[])
{
	return cmd_passthrough(sh, BT_AVRCP_OPID_PAUSE, NULL, 0);
}

static int cmd_get_cap(const struct shell *sh, int32_t argc, char *argv[])
{
	const char *cap_id;

	if (!avrcp_ct_registered && register_ct_cb(sh) != 0) {
		return -ENOEXEC;
	}

	if (default_ct == NULL) {
		shell_error(sh, "AVRCP is not connected");
		return 0;
	}

	cap_id = argv[1];
	if (!strcmp(cap_id, "company")) {
		bt_avrcp_ct_get_cap(default_ct, get_next_tid(), BT_AVRCP_CAP_COMPANY_ID);
	} else if (!strcmp(cap_id, "events")) {
		bt_avrcp_ct_get_cap(default_ct, get_next_tid(), BT_AVRCP_CAP_EVENTS_SUPPORTED);
	}

	return 0;
}

#define HELP_PASSTHROUGH_RSP                                                     \
	"send_passthrough_rsp <op/opvu> <opid> <state>\n"                        \
	"op/opvu: passthrough command (normal/passthrough VENDOR UNIQUE)\n"      \
	"opid: operation identifier (e.g., play/pause or hex value)\n"           \
	"state: [pressed|released]"

SHELL_STATIC_SUBCMD_SET_CREATE(
	ct_cmds,
	SHELL_CMD_ARG(register_cb, NULL, "register avrcp ct callbacks", cmd_register_ct_cb, 1, 0),
	SHELL_CMD_ARG(get_unit, NULL, "get unit info", cmd_get_unit_info, 1, 0),
	SHELL_CMD_ARG(get_subunit, NULL, "get subunit info", cmd_get_subunit_info, 1, 0),
	SHELL_CMD_ARG(get_cap, NULL, "get capabilities <cap_id: company or events>", cmd_get_cap, 2,
		      0),
	SHELL_CMD_ARG(play, NULL, "request a play at the remote player", cmd_play, 1, 0),
	SHELL_CMD_ARG(pause, NULL, "request a pause at the remote player", cmd_pause, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	tg_cmds,
	SHELL_CMD_ARG(register_cb, NULL, "register avrcp tg callbacks", cmd_register_tg_cb, 1, 0),
	SHELL_CMD_ARG(send_unit_rsp, NULL, "send unit info response", cmd_send_unit_info_rsp, 1, 0),
	SHELL_CMD_ARG(send_subunit_rsp, NULL, "send subunit info response",
		      cmd_send_subunit_info_rsp, 1, 0),
	SHELL_CMD_ARG(send_passthrough_rsp, NULL, HELP_PASSTHROUGH_RSP, cmd_send_passthrough_rsp,
		      4, 0),
	SHELL_SUBCMD_SET_END);

static int cmd_avrcp(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		/* sh returns 1 when help is printed */
		return 1;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	avrcp_cmds,
	SHELL_CMD_ARG(connect, NULL, "connect AVRCP", cmd_connect, 1, 0),
	SHELL_CMD_ARG(disconnect, NULL, "disconnect AVRCP", cmd_disconnect, 1, 0),
	SHELL_CMD(ct, &ct_cmds, "AVRCP CT shell commands", cmd_avrcp),
	SHELL_CMD(tg, &tg_cmds, "AVRCP TG shell commands", cmd_avrcp),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(avrcp, &avrcp_cmds, "Bluetooth AVRCP sh commands", cmd_avrcp, 1, 1);
