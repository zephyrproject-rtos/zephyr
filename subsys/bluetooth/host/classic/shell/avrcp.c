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

NET_BUF_POOL_DEFINE(avrcp_tx_pool, CONFIG_BT_MAX_CONN,
		    BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

#define FOLDER_NAME_HEX_BUF_LEN 80

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

static void avrcp_ct_browsing_connected(struct bt_conn *conn, struct bt_avrcp_ct *ct)
{
	bt_shell_print("AVRCP CT browsing connected");
}

static void avrcp_ct_browsing_disconnected(struct bt_avrcp_ct *ct)
{
	bt_shell_print("AVRCP CT browsing disconnected");
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

static void avrcp_browsed_player_rsp(struct bt_avrcp_ct *ct, uint8_t tid,
				     struct net_buf *buf)
{
	struct bt_avrcp_set_browsed_player_rsp *rsp;
	struct bt_avrcp_folder_name *folder_name;

	rsp = net_buf_pull_mem(buf, sizeof(*rsp));
	if (rsp->status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		bt_shell_print("AVRCP set browsed player failed, tid = %d, status = 0x%02x",
			       tid, rsp->status);
		return;
	}

	bt_shell_print("AVRCP set browsed player success, tid = %d", tid);
	bt_shell_print("  UID Counter: %u", sys_be16_to_cpu(rsp->uid_counter));
	bt_shell_print("  Number of Items: %u", sys_be32_to_cpu(rsp->num_items));
	bt_shell_print("  Charset ID: 0x%04X", sys_be16_to_cpu(rsp->charset_id));
	bt_shell_print("  Folder Depth: %u", rsp->folder_depth);

	while (buf->len > 0) {
		if (buf->len < sizeof(struct bt_avrcp_folder_name)) {
			bt_shell_print("incompleted message");
			break;
		}
		folder_name = net_buf_pull_mem(buf, sizeof(struct bt_avrcp_folder_name));
		folder_name->folder_name_len = sys_be16_to_cpu(folder_name->folder_name_len);
		if (buf->len < folder_name->folder_name_len) {
			bt_shell_print("incompleted message for folder_name");
			break;
		}
		net_buf_pull_mem(buf, folder_name->folder_name_len);

		if (sys_be16_to_cpu(rsp->charset_id) == BT_AVRCP_CHARSET_UTF8) {
			bt_shell_print("Raw folder name:");
			for (int i = 0; i < folder_name->folder_name_len; i++) {
				bt_shell_print("%c", folder_name->folder_name[i]);
			}
		} else {
			bt_shell_print(" Get folder Name : ");
			bt_shell_hexdump(folder_name->folder_name, folder_name->folder_name_len);
		}
		if (rsp->folder_depth > 0) {
			rsp->folder_depth--;
		} else {
			bt_shell_warn("Folder depth is mismatched with received data");
			break;
		}
	}

	if (rsp->folder_depth > 0) {
		bt_shell_print("folder depth mismatch: expected 0, got %u", rsp->folder_depth);
	}
}

static struct bt_avrcp_ct_cb app_avrcp_ct_cb = {
	.connected = avrcp_ct_connected,
	.disconnected = avrcp_ct_disconnected,
	.browsing_connected = avrcp_ct_browsing_connected,
	.browsing_disconnected = avrcp_ct_browsing_disconnected,
	.get_cap_rsp = avrcp_get_cap_rsp,
	.unit_info_rsp = avrcp_unit_info_rsp,
	.subunit_info_rsp = avrcp_subunit_info_rsp,
	.passthrough_rsp = avrcp_passthrough_rsp,
	.browsed_player_rsp = avrcp_browsed_player_rsp,
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

static void avrcp_tg_browsing_connected(struct bt_conn *conn, struct bt_avrcp_tg *tg)
{
	bt_shell_print("AVRCP TG browsing connected");
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

static void avrcp_tg_browsing_disconnected(struct bt_avrcp_tg *tg)
{
	bt_shell_print("AVRCP TG browsing disconnected");
}

static void avrcp_set_browsed_player_req(struct bt_avrcp_tg *tg, uint8_t tid,
					 uint16_t player_id)
{
	bt_shell_print("AVRCP set browsed player request received, player_id = %u", player_id);
	tg_tid = tid;
}

static void avrcp_passthrough_req(struct bt_avrcp_tg *tg, uint8_t tid, struct net_buf *buf)
{
	struct bt_avrcp_passthrough_cmd *cmd;
	struct bt_avrcp_passthrough_opvu_data *opvu = NULL;
	const char *state_str;
	bt_avrcp_opid_t opid;
	bt_avrcp_button_state_t state;

	tg_tid = tid;
	cmd = net_buf_pull_mem(buf, sizeof(*cmd));
	opid = BT_AVRCP_PASSTHROUGH_GET_STATE(cmd);
	state = BT_AVRCP_PASSTHROUGH_GET_OPID(cmd);

	if (cmd->data_len > 0U) {
		if (buf->len < sizeof(struct bt_avrcp_passthrough_opvu_data)) {
			bt_shell_print("Invalid passthrough data: buf length = %u, need >= %zu",
				       buf->len, sizeof(struct bt_avrcp_passthrough_opvu_data));
			return;
		}

		if (buf->len < cmd->data_len) {
			bt_shell_print("Invalid passthrough cmd data length: %u, buf length = %u",
				       cmd->data_len, buf->len);
		}
		opvu = net_buf_pull_mem(buf, sizeof(*opvu));
	}

	/* Convert button state to string */
	state_str = (state == BT_AVRCP_BUTTON_PRESSED) ? "PRESSED" : "RELEASED";

	bt_shell_print("AVRCP passthrough command received: opid = 0x%02x (%s), tid=0x%02x, len=%u",
		       opid, state_str, tid, cmd->data_len);

	if (cmd->data_len > 0U && opvu != NULL) {
		bt_shell_print("company_id: 0x%06x", sys_get_be24(opvu->company_id));
		bt_shell_print("opid_vu: 0x%04x", sys_be16_to_cpu(opvu->opid_vu));
	}

}

static struct bt_avrcp_tg_cb app_avrcp_tg_cb = {
	.connected = avrcp_tg_connected,
	.disconnected = avrcp_tg_disconnected,
	.browsing_connected = avrcp_tg_browsing_connected,
	.browsing_disconnected = avrcp_tg_browsing_disconnected,
	.unit_info_req = avrcp_unit_info_req,
	.subunit_info_req = avrcp_subunit_info_req,
	.set_browsed_player_req = avrcp_set_browsed_player_req,
	.passthrough_req = avrcp_passthrough_req,
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

static int cmd_browsing_connect(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;

	if (!avrcp_ct_registered && register_ct_cb(sh) != 0) {
		return -ENOEXEC;
	}

	if (default_conn == NULL) {
		shell_error(sh, "BR/EDR not connected");
		return -ENOEXEC;
	}

	err = bt_avrcp_browsing_connect(default_conn);
	if (err < 0) {
		shell_error(sh, "fail to connect AVRCP browsing");
	} else {
		shell_print(sh, "AVRCP browsing connect request sent");
	}

	return err;
}

static int cmd_browsing_disconnect(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;

	if (default_conn == NULL) {
		shell_print(sh, "Not connected");
		return -ENOEXEC;
	}

	if ((default_ct != NULL) || (default_tg != NULL)) {
		err = bt_avrcp_browsing_disconnect(default_conn);
		if (err < 0) {
			shell_error(sh, "fail to disconnect AVRCP browsing");
		} else {
			shell_print(sh, "AVRCP browsing disconnect request sent");
		}
	} else {
		shell_error(sh, "AVRCP is not connected");
		err = -ENOEXEC;
	}

	return err;
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

static int cmd_send_passthrough_rsp(const struct shell *sh, int32_t argc, char *argv[])
{
	struct bt_avrcp_passthrough_rsp *rsp;
	struct bt_avrcp_passthrough_opvu_data *opvu = NULL;
	bt_avrcp_opid_t opid = 0;
	bt_avrcp_button_state_t state;
	uint16_t vu_opid = 0;
	bool is_op_vu = true;
	struct net_buf *buf;
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

	buf = bt_avrcp_create_pdu(NULL);
	if (buf == NULL) {
		shell_error(sh, "Failed to allocate buffer for AVRCP passthrough response");
		return -ENOMEM;
	}

	if (net_buf_tailroom(buf) < sizeof(struct bt_avrcp_passthrough_rsp)) {
		shell_error(sh, "Not enough tailroom in buffer for passthrough rsp");
		goto failed;
	}
	rsp = net_buf_add(buf, sizeof(*rsp));

	if (!strcmp(argv[1], "op")) {
		is_op_vu = false;
	} else if (!strcmp(argv[1], "opvu")) {
		is_op_vu = true;
	} else {
		shell_error(sh, "Invalid response: %s", argv[1]);
		goto failed;
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
			goto failed;
		}
		if (is_op_vu) {
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
		goto failed;
	}

	if (is_op_vu) {
		opid = BT_AVRCP_OPID_VENDOR_UNIQUE;
	}

	BT_AVRCP_PASSTHROUGH_SET_STATE_OPID(rsp, state, opid);
	if (is_op_vu) {
		if (net_buf_tailroom(buf) < sizeof(*opvu)) {
			shell_error(sh, "Not enough tailroom in buffer for opvu");
			goto failed;
		}
		opvu = net_buf_add(buf, sizeof(*opvu));
		sys_put_be24(BT_AVRCP_COMPANY_ID_BLUETOOTH_SIG, opvu->company_id);
		opvu->opid_vu = sys_cpu_to_be16(vu_opid);
		rsp->data_len = sizeof(*opvu);
	} else {
		rsp->data_len = 0;
	}

	err = bt_avrcp_tg_send_passthrough_rsp(default_tg, tg_tid, BT_AVRCP_RSP_ACCEPTED, buf);
	if (err < 0) {
		shell_error(sh, "Failed to send passthrough response: %d", err);
		goto failed;
	} else {
		shell_print(sh, "Passthrough opid=0x%02x, state=%s", opid, argv[2]);
		return 0;
	}

failed:
	net_buf_unref(buf);
	return -ENOEXEC;
}

static int cmd_send_subunit_info_rsp(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;

	if (!avrcp_tg_registered && register_tg_cb(sh) != 0) {
		return -ENOEXEC;
	}

	if (default_tg != NULL) {
		err = bt_avrcp_tg_send_subunit_info_rsp(default_tg, tg_tid);
		if (err == 0) {
			shell_print(sh, "AVRCP send subunit info response");
		} else {
			shell_error(sh, "Failed to send subunit info response");
		}
	} else {
		shell_error(sh, "AVRCP is not connected");
	}

	return 0;
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

static int cmd_set_browsed_player(const struct shell *sh, int32_t argc, char *argv[])
{
	uint16_t player_id;
	int err;

	if (!avrcp_ct_registered && register_ct_cb(sh) != 0) {
		return -ENOEXEC;
	}

	if (default_ct == NULL) {
		shell_error(sh, "AVRCP is not connected");
		return -ENOEXEC;
	}

	player_id = (uint16_t)strtoul(argv[1], NULL, 0);

	err = bt_avrcp_ct_set_browsed_player(default_ct, get_next_tid(), player_id);
	if (err < 0) {
		shell_error(sh, "fail to set browsed player");
	} else {
		shell_print(sh, "AVRCP send set browsed player req");
	}

	return 0;
}

static int cmd_send_set_browsed_player_rsp(const struct shell *sh, int32_t argc, char *argv[])
{
	struct bt_avrcp_set_browsed_player_rsp *rsp;
	struct bt_avrcp_folder_name *folder_name;
	char *folder_name_str = "Music";
	uint8_t folder_name_hex[FOLDER_NAME_HEX_BUF_LEN];
	uint16_t folder_name_len = 0;
	struct net_buf *buf;
	uint16_t param_len;
	int err;

	if (!avrcp_tg_registered && register_tg_cb(sh) != 0) {
		return -ENOEXEC;
	}

	if (default_tg == NULL) {
		shell_error(sh, "AVRCP TG is not connected");
		return -ENOEXEC;
	}

	buf = bt_avrcp_create_pdu(&avrcp_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Failed to allocate buffer for AVRCP browsing response");
		return -ENOMEM;
	}

	if (net_buf_tailroom(buf) < sizeof(struct bt_avrcp_set_browsed_player_rsp)) {
		shell_error(sh, "Not enough tailroom in buffer for browsed player rsp");
		goto failed;
	}

	rsp = net_buf_add(buf, sizeof(*rsp));
	/* Set default rsp */
	rsp->status = BT_AVRCP_STATUS_OPERATION_COMPLETED;
	rsp->uid_counter = sys_cpu_to_be16(0x0001U);
	rsp->num_items = sys_cpu_to_be32(100U);
	rsp->charset_id = sys_cpu_to_be16(BT_AVRCP_CHARSET_UTF8);
	rsp->folder_depth = 1;

	/* Parse command line arguments or use default values */
	if (argc >= 2) {
		rsp->status = (uint8_t)strtoul(argv[1], NULL, 0);
	}

	if (argc >= 3) {
		rsp->uid_counter = sys_cpu_to_be16((uint16_t)strtoul(argv[2], NULL, 0));
	}

	if (argc >= 4) {
		rsp->num_items = sys_cpu_to_be32((uint32_t)strtoul(argv[3], NULL, 0));
	}

	if (argc >= 5) {
		rsp->charset_id = sys_cpu_to_be16((uint16_t)strtoul(argv[4], NULL, 0));
	}

	if (rsp->charset_id == sys_cpu_to_be16(BT_AVRCP_CHARSET_UTF8)) {
		if (argc >= 6) {
			folder_name_str = argv[5];
		}
		folder_name_len = strlen(folder_name_str);
	} else {
		if (argc >= 6) {
			folder_name_len = hex2bin(argv[5], strlen(argv[5]), folder_name_hex,
						  sizeof(folder_name_hex));
			if (folder_name_len == 0) {
				shell_error(sh, "Failed to get folder_name from  %s", argv[5]);
			}
		} else {
			shell_error(sh, "Please input hex string for folder_name");
			goto failed;
		}
	}

	param_len = folder_name_len + sizeof(struct bt_avrcp_folder_name);
	if (net_buf_tailroom(buf) < param_len) {
		shell_error(sh, "Not enough tailroom in buffer for param");
		goto failed;
	}

	folder_name = net_buf_add(buf, sizeof(*folder_name));
	folder_name->folder_name_len = sys_cpu_to_be16(folder_name_len);
	if (rsp->charset_id == sys_cpu_to_be16(BT_AVRCP_CHARSET_UTF8)) {
		net_buf_add_mem(buf, folder_name_str, folder_name_len);
	} else {
		net_buf_add_mem(buf, folder_name_hex, folder_name_len);
	}

	err = bt_avrcp_tg_send_set_browsed_player_rsp(default_tg, tg_tid, buf);
	if (err == 0) {
		shell_print(sh, "Send set browsed player response, status = 0x%02x", rsp->status);
	} else {
		shell_error(sh, "Failed to send set browsed player response, err = %d", err);
		goto failed;
	}

	return 0;
failed:
	net_buf_unref(buf);
	return -ENOEXEC;
}

#define HELP_NONE "[none]"
#define HELP_PASSTHROUGH_RSP                                                     \
	"send_passthrough_rsp <op/opvu> <opid> <state>\n"                        \
	"op/opvu: passthrough command (normal/passthrough VENDOR UNIQUE)\n"      \
	"opid: operation identifier (e.g., play/pause or hex value)\n"           \
	"state: [pressed|released]"

#define HELP_BROWSED_PLAYER_RSP                                                      \
	"Send SetBrowsedPlayer response\n"					     \
	"Usage: send_browsed_player_rsp [status] [uid_counter] [num_items] "         \
	"[charset_id] [folder_name]"

SHELL_STATIC_SUBCMD_SET_CREATE(
	ct_cmds,
	SHELL_CMD_ARG(register_cb, NULL, "register avrcp ct callbacks", cmd_register_ct_cb, 1, 0),
	SHELL_CMD_ARG(get_unit, NULL, "get unit info", cmd_get_unit_info, 1, 0),
	SHELL_CMD_ARG(get_subunit, NULL, "get subunit info", cmd_get_subunit_info, 1, 0),
	SHELL_CMD_ARG(get_cap, NULL, "get capabilities <cap_id: company or events>", cmd_get_cap, 2,
		      0),
	SHELL_CMD_ARG(play, NULL, "request a play at the remote player", cmd_play, 1, 0),
	SHELL_CMD_ARG(pause, NULL, "request a pause at the remote player", cmd_pause, 1, 0),
	SHELL_CMD_ARG(set_browsed_player, NULL, "set browsed player <player_id>",
		      cmd_set_browsed_player, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	tg_cmds,
	SHELL_CMD_ARG(register_cb, NULL, "register avrcp tg callbacks", cmd_register_tg_cb, 1, 0),
	SHELL_CMD_ARG(send_unit_rsp, NULL, "send unit info response", cmd_send_unit_info_rsp, 1, 0),
	SHELL_CMD_ARG(send_subunit_rsp, NULL, HELP_NONE, cmd_send_subunit_info_rsp, 1, 0),
	SHELL_CMD_ARG(send_browsed_player_rsp, NULL, HELP_BROWSED_PLAYER_RSP,
		      cmd_send_set_browsed_player_rsp, 1, 5),
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
	SHELL_CMD_ARG(browsing_connect, NULL, "connect browsing AVRCP", cmd_browsing_connect, 1, 0),
	SHELL_CMD_ARG(browsing_disconnect, NULL, "disconnect browsing AVRCP",
		      cmd_browsing_disconnect, 1, 0),
	SHELL_CMD(ct, &ct_cmds, "AVRCP CT shell commands", cmd_avrcp),
	SHELL_CMD(tg, &tg_cmds, "AVRCP TG shell commands", cmd_avrcp),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(avrcp, &avrcp_cmds, "Bluetooth AVRCP sh commands", cmd_avrcp, 1, 1);
