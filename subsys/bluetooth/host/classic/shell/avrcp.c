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
static uint8_t tg_cap_id;

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
static struct bt_avrcp_media_attr test_media_attrs[] = {
	{
		.attr_id = BT_AVRCP_MEDIA_ATTR_TITLE,
		.charset_id = BT_AVRCP_CHARSET_UTF8,
		.attr_len = 11U,
		.attr_val = (const uint8_t *)"Test Title",
	},
	{
		.attr_id = BT_AVRCP_MEDIA_ATTR_ARTIST,
		.charset_id = BT_AVRCP_CHARSET_UTF8,
		.attr_len = 11U,
		.attr_val = (const uint8_t *)"Test Artist",
	},
	{
		.attr_id = BT_AVRCP_MEDIA_ATTR_ALBUM,
		.charset_id = BT_AVRCP_CHARSET_UTF8,
		.attr_len = 10U,
		.attr_val = (const uint8_t *)"Test Album",
	},
	{
		.attr_id = BT_AVRCP_MEDIA_ATTR_TRACK_NUMBER,
		.charset_id = BT_AVRCP_CHARSET_UTF8,
		.attr_len = 1U,
		.attr_val = (const uint8_t *)"1",
	},
	{
		.attr_id = BT_AVRCP_MEDIA_ATTR_TOTAL_TRACKS,
		.charset_id = BT_AVRCP_CHARSET_UTF8,
		.attr_len = 2U,
		.attr_val = (const uint8_t *)"10",
	},
	{
		.attr_id = BT_AVRCP_MEDIA_ATTR_GENRE,
		.charset_id = BT_AVRCP_CHARSET_UTF8,
		.attr_len = 4U,
		.attr_val = (const uint8_t *)"Rock",
	},
	{
		.attr_id = BT_AVRCP_MEDIA_ATTR_PLAYING_TIME,
		.charset_id = BT_AVRCP_CHARSET_UTF8,
		.attr_len = 6U,
		.attr_val = (const uint8_t *)"240000", /* 4 minutes in milliseconds */
	},
};
static struct bt_avrcp_media_attr large_media_attrs[] = {
	{
		.attr_id = BT_AVRCP_MEDIA_ATTR_TITLE,
		.charset_id = BT_AVRCP_CHARSET_UTF8,
		.attr_len = 200U,
		.attr_val = (const uint8_t *)
		"This is a long title that is designed to test the fragmentation of the AVRCP.",
	},
	{
		.attr_id = BT_AVRCP_MEDIA_ATTR_ARTIST,
		.charset_id = BT_AVRCP_CHARSET_UTF8,
		.attr_len = 150U,
		.attr_val = (const uint8_t *)
		"This is a very long artist name that is also designed to test fragmentation.",
	},
	{
		.attr_id = BT_AVRCP_MEDIA_ATTR_ALBUM,
		.charset_id = BT_AVRCP_CHARSET_UTF8,
		.attr_len = 100U,
		.attr_val = (const uint8_t *)
		"This is a long album name for testing fragmentation of AVRCP responses.",
	},
};

static void avrcp_get_element_attrs_rsp(struct bt_avrcp_ct *ct, uint8_t tid,
					 const struct bt_avrcp_get_element_attrs_rsp *rsp)
{
	uint8_t i;
	const char *attr_name;

	bt_shell_print("AVRCP GetElementAttributes response received, tid=0x%02x, num_attrs=%u",
		       tid, rsp->num_attrs);

	for (i = 0U; i < rsp->num_attrs; i++) {
		const struct bt_avrcp_media_attr *attr = &rsp->attrs[i];

		/* Convert attribute ID to string for display */
		switch (attr->attr_id) {
		case BT_AVRCP_MEDIA_ATTR_TITLE:
			attr_name = "TITLE";
			break;
		case BT_AVRCP_MEDIA_ATTR_ARTIST:
			attr_name = "ARTIST";
			break;
		case BT_AVRCP_MEDIA_ATTR_ALBUM:
			attr_name = "ALBUM";
			break;
		case BT_AVRCP_MEDIA_ATTR_TRACK_NUMBER:
			attr_name = "TRACK_NUMBER";
			break;
		case BT_AVRCP_MEDIA_ATTR_TOTAL_TRACKS:
			attr_name = "TOTAL_TRACKS";
			break;
		case BT_AVRCP_MEDIA_ATTR_GENRE:
			attr_name = "GENRE";
			break;
		case BT_AVRCP_MEDIA_ATTR_PLAYING_TIME:
			attr_name = "PLAYING_TIME";
			break;
		default:
			attr_name = "UNKNOWN";
			break;
		}

		bt_shell_print("  Attr[%u]: ID=0x%08x (%s), charset=0x%04x, len=%u",
			       i, attr->attr_id, attr_name, attr->charset_id, attr->attr_len);

		/* Print attribute value (truncate if too long for display) */
		if (attr->attr_len > 0U && attr->attr_val != NULL) {
			uint16_t print_len = (attr->attr_len > 64U) ? 64U : attr->attr_len;
			char value_str[65];

			memcpy(value_str, attr->attr_val, print_len);
			value_str[print_len] = '\0';
			bt_shell_print("    Value: \"%s\"%s", value_str,
				       (attr->attr_len > 64U) ? "..." : "");
		}
	}
}
static void avrcp_get_element_attrs_cmd_req(struct bt_avrcp_tg *tg, uint8_t tid,
					     const struct bt_avrcp_get_element_attrs_cmd *cmd)
{
	uint8_t i;

	bt_shell_print("AVRCP GetElementAttributes command received, tid=0x%02x", tid);
	bt_shell_print("  Identifier: 0x%016llx", cmd->identifier);
	bt_shell_print("  Num attrs requested: %u %s", cmd->num_attrs,
		       (cmd->num_attrs == 0U) ? "(all attributes)" : "");

	if (cmd->num_attrs > 0U && cmd->attr_ids != NULL) {
		bt_shell_print("  Requested attribute IDs:");
		for (i = 0U; i < cmd->num_attrs; i++) {
			const char *attr_name;

			switch (cmd->attr_ids[i]) {
			case BT_AVRCP_MEDIA_ATTR_TITLE:
				attr_name = "TITLE";
				break;
			case BT_AVRCP_MEDIA_ATTR_ARTIST:
				attr_name = "ARTIST";
				break;
			case BT_AVRCP_MEDIA_ATTR_ALBUM:
				attr_name = "ALBUM";
				break;
			case BT_AVRCP_MEDIA_ATTR_TRACK_NUMBER:
				attr_name = "TRACK_NUMBER";
				break;
			case BT_AVRCP_MEDIA_ATTR_TOTAL_TRACKS:
				attr_name = "TOTAL_TRACKS";
				break;
			case BT_AVRCP_MEDIA_ATTR_GENRE:
				attr_name = "GENRE";
				break;
			case BT_AVRCP_MEDIA_ATTR_PLAYING_TIME:
				attr_name = "PLAYING_TIME";
				break;
			default:
				attr_name = "UNKNOWN";
				break;
			}

			bt_shell_print("    [%u]: 0x%08x (%s)", i, cmd->attr_ids[i], attr_name);
		}
	}

	/* Store the transaction ID for manual response testing */
	tg_tid = tid;
}

static struct bt_avrcp_ct_cb app_avrcp_ct_cb = {
	.connected = avrcp_ct_connected,
	.disconnected = avrcp_ct_disconnected,
	.get_cap_rsp = avrcp_get_cap_rsp,
	.unit_info_rsp = avrcp_unit_info_rsp,
	.subunit_info_rsp = avrcp_subunit_info_rsp,
	.passthrough_rsp = avrcp_passthrough_rsp,
	.get_element_attrs_rsp = avrcp_get_element_attrs_rsp,
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

static void avrcp_get_cap_cmd_req(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t cap_id)
{
	const char *cap_type_str;

	/* Convert capability ID to string for display */
	switch (cap_id) {
	case BT_AVRCP_CAP_COMPANY_ID:
		cap_type_str = "COMPANY_ID";
		break;
	case BT_AVRCP_CAP_EVENTS_SUPPORTED:
		cap_type_str = "EVENTS_SUPPORTED";
		break;
	default:
		cap_type_str = "UNKNOWN";
		break;
	}

	bt_shell_print("AVRCP get capabilities command received: cap_id 0x%02x (%s), tid = 0x%02x",
		       cap_id, cap_type_str, tid);

	/* Store the transaction ID and capability ID for manual response testing */
	tg_tid = tid;
	tg_cap_id = cap_id;
}
static struct bt_avrcp_tg_cb app_avrcp_tg_cb = {
	.connected = avrcp_tg_connected,
	.disconnected = avrcp_tg_disconnected,
	.unit_info_req = avrcp_unit_info_req,
	.get_cap_cmd_req = avrcp_get_cap_cmd_req,
	.get_element_attrs_cmd_req = avrcp_get_element_attrs_cmd_req,
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

static int cmd_send_get_cap_rsp(const struct shell *sh, int32_t argc, char *argv[])
{
	struct bt_avrcp_get_cap_rsp *rsp;
	uint8_t rsp_buffer[32U];
	uint8_t *cap_data;
	int err;

	if (!avrcp_tg_registered && register_tg_cb(sh) != 0) {
		return -ENOEXEC;
	}

	if (default_tg == NULL) {
		shell_error(sh, "AVRCP TG is not connected");
		return -ENOEXEC;
	}

	/* Initialize response structure */
	rsp = (struct bt_avrcp_get_cap_rsp *)rsp_buffer;
	rsp->cap_id = tg_cap_id;
	cap_data = rsp->cap;

	switch (tg_cap_id) {
	case BT_AVRCP_CAP_COMPANY_ID:
		/* Send Bluetooth SIG company ID as example */
		rsp->cap_cnt = 1U;
		sys_put_be24(BT_AVRCP_COMPANY_ID_BLUETOOTH_SIG, cap_data);
		shell_print(sh, "Sending company ID capability response: 0x%06x",
			    BT_AVRCP_COMPANY_ID_BLUETOOTH_SIG);
		break;

	case BT_AVRCP_CAP_EVENTS_SUPPORTED:
		/* Send supported events as example */
		rsp->cap_cnt = 5U;
		cap_data[0] = BT_AVRCP_EVT_PLAYBACK_STATUS_CHANGED;
		cap_data[1] = BT_AVRCP_EVT_TRACK_CHANGED;
		cap_data[2] = BT_AVRCP_EVT_TRACK_REACHED_END;
		cap_data[3] = BT_AVRCP_EVT_TRACK_REACHED_START;
		cap_data[4] = BT_AVRCP_EVT_VOLUME_CHANGED;
		shell_print(sh, "Sending events supported capability response with %u events",
			    rsp->cap_cnt);
		break;

	default:
		shell_error(sh, "Unknown capability ID: 0x%02x", tg_cap_id);
		return -EINVAL;
	}

	err = bt_avrcp_tg_send_get_cap_rsp(default_tg, tg_tid, rsp);
	if (err) {
		shell_error(sh, "Failed to send get capabilities response: %d", err);
	} else {
		shell_print(sh, "Get capabilities response sent successfully");
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

static int cmd_get_element_attrs(const struct shell *sh, int32_t argc, char *argv[])
{
	uint64_t identifier = 0U;
	uint32_t attr_ids[7];
	uint8_t num_attrs = 0U;
	uint32_t *attr_ids_ptr = NULL;
	char *endptr;
	unsigned long val;
	int i;

	if (!avrcp_ct_registered && register_ct_cb(sh) != 0) {
		return -ENOEXEC;
	}

	if (default_ct == NULL) {
		shell_error(sh, "AVRCP CT is not connected");
		return -ENOEXEC;
	}

	/* Parse optional identifier */
	if (argc > 1) {
		identifier = strtoull(argv[1], &endptr, 16);
		if (*endptr != '\0') {
			shell_error(sh, "Invalid identifier: %s", argv[1]);
			return -EINVAL;
		}
	}

	/* Parse optional attribute IDs */
	if (argc > 2 && identifier != 0) {
		for (i = 2; i < argc && i < 9; i++) { /* Max 7 attributes + cmd + identifier */
			val = strtoul(argv[i], &endptr, 16);
			if (*endptr != '\0' || val > 0xFFFFFFFFUL) {
				shell_error(sh, "Invalid attribute ID: %s", argv[i]);
				return -EINVAL;
			}
			attr_ids[num_attrs++] = (uint32_t)val;
		}
		attr_ids_ptr = attr_ids;
	}

	shell_print(sh, "Requesting element attributes: identifier=0x%016llx, num_attrs=%u",
		    identifier, num_attrs);

	return bt_avrcp_ct_get_element_attrs(default_ct, get_next_tid(), identifier,
					     attr_ids_ptr, num_attrs);
}

static int cmd_send_get_element_attrs_rsp(const struct shell *sh, int32_t argc, char *argv[])
{
	struct bt_avrcp_get_element_attrs_rsp rsp;
	bool use_large_attrs = false;
	char *endptr;
	int err;

	if (!avrcp_tg_registered && register_tg_cb(sh) != 0) {
		return -ENOEXEC;
	}

	if (default_tg == NULL) {
		shell_error(sh, "AVRCP TG is not connected");
		return -ENOEXEC;
	}

	if (argc > 1) {
		use_large_attrs = strtoull(argv[1], &endptr, 16);
		if (*endptr != '\0') {
			shell_error(sh, "Invalid identifier: %s", argv[1]);
			return -EINVAL;
		}
	}

	/* Determine which attribute set to use */
	if (use_large_attrs) {
		rsp.num_attrs = ARRAY_SIZE(large_media_attrs);
		rsp.attrs = large_media_attrs;
		shell_print(sh, "Sending large Attributes response (%u attrs) for fragment test",
			    rsp.num_attrs);
	} else {
		rsp.num_attrs = ARRAY_SIZE(test_media_attrs);
		rsp.attrs = test_media_attrs;
		shell_print(sh, "Sending standard GetElementAttributes response (%u attrs)",
			    rsp.num_attrs);
	}

	err = bt_avrcp_tg_send_get_element_attrs_rsp(default_tg, tg_tid, &rsp);
	if (err) {
		shell_error(sh, "Failed to send GetElementAttributes response: %d", err);
	} else {
		shell_print(sh, "GetElementAttributes response sent successfully");
	}

	return err;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	ct_cmds,
	SHELL_CMD_ARG(register_cb, NULL, "register avrcp ct callbacks", cmd_register_ct_cb, 1, 0),
	SHELL_CMD_ARG(get_unit, NULL, "get unit info", cmd_get_unit_info, 1, 0),
	SHELL_CMD_ARG(get_subunit, NULL, "get subunit info", cmd_get_subunit_info, 1, 0),
	SHELL_CMD_ARG(get_cap, NULL, "get capabilities <cap_id: company or events>", cmd_get_cap, 2,
		      0),
	SHELL_CMD_ARG(play, NULL, "request a play at the remote player", cmd_play, 1, 0),
	SHELL_CMD_ARG(pause, NULL, "request a pause at the remote player", cmd_pause, 1, 0),
	SHELL_CMD_ARG(get_element_attrs, NULL, "get element attrs [identifier] [attr1] [attr2] ...",
		      cmd_get_element_attrs, 1, 9),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	tg_cmds,
	SHELL_CMD_ARG(register_cb, NULL, "register avrcp tg callbacks", cmd_register_tg_cb, 1, 0),
	SHELL_CMD_ARG(send_unit_rsp, NULL, "send unit info response", cmd_send_unit_info_rsp, 1, 0),
	SHELL_CMD_ARG(send_get_cap_rsp, NULL, "send get capabilities response",
		      cmd_send_get_cap_rsp, 1, 0),
	SHELL_CMD_ARG(send_get_element_attrs_rsp, NULL, "send get element attrs response<large: 1>",
		      cmd_send_get_element_attrs_rsp, 2, 0),
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
