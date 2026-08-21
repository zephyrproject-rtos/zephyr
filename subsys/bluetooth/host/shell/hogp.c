/** @file
 *  @brief Bluetooth HOGP Device shell commands.
 */

/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hogp_device.h>
#include <zephyr/bluetooth/services/hids.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <zephyr/usb/class/hid.h>

#include "common/bt_shell_private.h"
#include "common/bt_str.h"
#include "host/shell/bt.h"

#define HELP_NONE "[none]"

/* Report IDs of the built-in Report Map below. */
#define HOGP_SHELL_REPORT_ID_MOUSE   0x01
#define HOGP_SHELL_REPORT_ID_LED     0x02
#define HOGP_SHELL_REPORT_ID_FEATURE 0x03

#define HOGP_SHELL_MOUSE_REPORT_LEN 3
#define HOGP_SHELL_MAX_VALUE_LEN    8

/* Mouse (Report ID 1, Input) + Keyboard LEDs (Report ID 2, Output) +
 * vendor defined (Report ID 3, Feature).
 */
static const uint8_t hogp_shell_report_map[] = {
	HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
	HID_USAGE(HID_USAGE_GEN_DESKTOP_MOUSE),
	HID_COLLECTION(HID_COLLECTION_APPLICATION),
		HID_REPORT_ID(HOGP_SHELL_REPORT_ID_MOUSE),
		HID_USAGE(HID_USAGE_GEN_DESKTOP_POINTER),
		HID_COLLECTION(HID_COLLECTION_PHYSICAL),
			HID_USAGE_PAGE(HID_USAGE_GEN_BUTTON),
			HID_USAGE_MIN8(1),
			HID_USAGE_MAX8(3),
			HID_LOGICAL_MIN8(0),
			HID_LOGICAL_MAX8(1),
			HID_REPORT_SIZE(1),
			HID_REPORT_COUNT(3),
			HID_INPUT(0x02),
			HID_REPORT_SIZE(5),
			HID_REPORT_COUNT(1),
			HID_INPUT(0x03),
			HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
			HID_USAGE(HID_USAGE_GEN_DESKTOP_X),
			HID_USAGE(HID_USAGE_GEN_DESKTOP_Y),
			HID_LOGICAL_MIN8(-127),
			HID_LOGICAL_MAX8(127),
			HID_REPORT_SIZE(8),
			HID_REPORT_COUNT(2),
			HID_INPUT(0x06),
		HID_END_COLLECTION,
	HID_END_COLLECTION,

	HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
	HID_USAGE(HID_USAGE_GEN_DESKTOP_KEYBOARD),
	HID_COLLECTION(HID_COLLECTION_APPLICATION),
		HID_REPORT_ID(HOGP_SHELL_REPORT_ID_LED),
		HID_USAGE_PAGE(HID_USAGE_GEN_LEDS),
		HID_USAGE_MIN8(1),
		HID_USAGE_MAX8(5),
		HID_REPORT_SIZE(1),
		HID_REPORT_COUNT(5),
		HID_OUTPUT(0x02),
		HID_REPORT_SIZE(3),
		HID_REPORT_COUNT(1),
		HID_OUTPUT(0x03),
		HID_REPORT_ID(HOGP_SHELL_REPORT_ID_FEATURE),
		HID_USAGE_PAGE16(0xFF00),
		HID_USAGE(0x01),
		HID_LOGICAL_MIN8(0),
		HID_LOGICAL_MAX16(0xFF, 0x00),
		HID_REPORT_SIZE(8),
		HID_REPORT_COUNT(1),
		HID_FEATURE(0x02),
	HID_END_COLLECTION,
};

/* Report values returned to the Host on GET_REPORT and updated on SET_REPORT. */
static struct hogp_shell_value {
	uint8_t id;
	uint8_t len;
	uint8_t data[HOGP_SHELL_MAX_VALUE_LEN];
} hogp_shell_values[] = {
	{.id = HOGP_SHELL_REPORT_ID_MOUSE, .len = HOGP_SHELL_MOUSE_REPORT_LEN},
	{.id = HOGP_SHELL_REPORT_ID_LED, .len = 1},
	{.id = HOGP_SHELL_REPORT_ID_FEATURE, .len = 1},
};

static bool hogp_registered;

static struct hogp_shell_value *value_lookup(uint8_t report_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(hogp_shell_values); i++) {
		if (hogp_shell_values[i].id == report_id) {
			return &hogp_shell_values[i];
		}
	}

	return NULL;
}

static ssize_t hogp_get_report(struct bt_conn *conn, uint8_t report_type, uint8_t report_id,
			       uint8_t *buf, uint16_t buf_size)
{
	const struct hogp_shell_value *value = value_lookup(report_id);

	ARG_UNUSED(conn);

	if (value == NULL) {
		bt_shell_warn("HOGP: GET_REPORT for unknown report ID %u", report_id);
		return -ENOENT;
	}

	if (value->len > buf_size) {
		return -ENOMEM;
	}

	bt_shell_print("HOGP: GET_REPORT type %u id %u", report_type, report_id);
	memcpy(buf, value->data, value->len);

	return value->len;
}

static void hogp_set_report(struct bt_conn *conn, uint8_t report_type, uint8_t report_id,
			    const uint8_t *data, uint16_t len)
{
	struct hogp_shell_value *value = value_lookup(report_id);

	ARG_UNUSED(conn);

	bt_shell_print("HOGP: SET_REPORT type %u id %u: %s", report_type, report_id,
		       bt_hex(data, len));

	if (value != NULL && len <= sizeof(value->data)) {
		memcpy(value->data, data, len);
		value->len = (uint8_t)len;
	}
}

static void hogp_protocol_mode_changed(struct bt_conn *conn, uint8_t protocol)
{
	ARG_UNUSED(conn);

	bt_shell_print("HOGP: Protocol Mode changed to %s",
		       protocol == BT_HID_PROTOCOL_BOOT ? "Boot" : "Report");
}

static void hogp_suspend_changed(struct bt_conn *conn, bool suspended)
{
	ARG_UNUSED(conn);

	bt_shell_print("HOGP: %s", suspended ? "Suspended" : "Exited suspend");
}

static void hogp_ccc_changed(struct bt_conn *conn, uint8_t report_id, uint8_t report_type,
			     bool enabled)
{
	ARG_UNUSED(conn);

	bt_shell_print("HOGP: Notifications %s for report type %u id %u",
		       enabled ? "enabled" : "disabled", report_type, report_id);
}

static const struct bt_hids_cb hogp_shell_cb = {
	.get_report = hogp_get_report,
	.set_report = hogp_set_report,
	.protocol_mode_changed = hogp_protocol_mode_changed,
	.suspend_changed = hogp_suspend_changed,
	.ccc_changed = hogp_ccc_changed,
};

static int cmd_register(const struct shell *sh, size_t argc, char *argv[])
{
	const struct bt_hogp_device_register_param param = {
		.hids = {
			.info = {
				.bcd_hid = 0x0111,
				.b_country_code = 0x00,
				.flags = BT_HID_INFO_FLAG_REMOTE_WAKE |
					 BT_HID_INFO_FLAG_NORMALLY_CONNECTABLE,
			},
			.report_map = hogp_shell_report_map,
			.report_map_len = sizeof(hogp_shell_report_map),
			.input_report_ids = {HOGP_SHELL_REPORT_ID_MOUSE},
			.output_report_ids = {HOGP_SHELL_REPORT_ID_LED},
			.feature_report_ids = {HOGP_SHELL_REPORT_ID_FEATURE},
			.cb = &hogp_shell_cb,
		},
	};
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	err = bt_hogp_device_register(&param);
	if (err != 0) {
		shell_error(sh, "Failed to register HOGP Device (err %d)", err);
		return -ENOEXEC;
	}

	hogp_registered = true;
	shell_print(sh, "HOGP Device registered");

	return 0;
}

static int cmd_unregister(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!hogp_registered) {
		shell_error(sh, "HOGP Device not registered");
		return -ENOEXEC;
	}

	err = bt_hogp_device_unregister();
	if (err != 0) {
		shell_error(sh, "Failed to unregister HOGP Device (err %d)", err);
		return -ENOEXEC;
	}

	hogp_registered = false;
	shell_print(sh, "HOGP Device unregistered");

	return 0;
}

static int cmd_info(const struct shell *sh, size_t argc, char *argv[])
{
	enum bt_hid_protocol_mode mode;
	bool suspended;
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Registered:    %s", hogp_registered ? "yes" : "no");
	if (!hogp_registered) {
		return 0;
	}

	if (default_conn == NULL) {
		shell_print(sh, "Not connected, no per-connection HID state");
	} else {
		err = bt_hogp_device_get_protocol_mode(default_conn, &mode);
		if (err != 0) {
			shell_error(sh, "Failed to get Protocol Mode (err %d)", err);
			return -ENOEXEC;
		}

		err = bt_hogp_device_get_suspend_state(default_conn, &suspended);
		if (err != 0) {
			shell_error(sh, "Failed to get Suspend state (err %d)", err);
			return -ENOEXEC;
		}

		shell_print(sh, "Protocol Mode: %s",
			    mode == BT_HID_PROTOCOL_BOOT ? "Boot" : "Report");
		shell_print(sh, "Suspended:     %s", suspended ? "yes" : "no");
	}

	for (size_t i = 0; i < ARRAY_SIZE(hogp_shell_values); i++) {
		const struct hogp_shell_value *value = &hogp_shell_values[i];

		shell_print(sh, "Report %u:      %s", value->id,
			    bt_hex(value->data, value->len));
	}

	return 0;
}

static int cmd_mouse(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t data[HOGP_SHELL_MOUSE_REPORT_LEN];
	unsigned long buttons;
	long delta;
	int err = 0;

	if (!hogp_registered) {
		shell_error(sh, "HOGP Device not registered");
		return -ENOEXEC;
	}

	buttons = shell_strtoul(argv[1], 16, &err);
	if (err != 0 || buttons > 0x07) {
		shell_error(sh, "Invalid buttons '%s' (0x00 - 0x07)", argv[1]);
		return -EINVAL;
	}
	data[0] = (uint8_t)buttons;

	for (size_t i = 0; i < 2; i++) {
		delta = shell_strtol(argv[2 + i], 10, &err);
		if (err != 0 || !IN_RANGE(delta, -127, 127)) {
			shell_error(sh, "Invalid delta '%s' (-127 - 127)", argv[2 + i]);
			return -EINVAL;
		}
		data[1 + i] = (uint8_t)(int8_t)delta;
	}

	err = bt_hogp_device_send_report(default_conn, HOGP_SHELL_REPORT_ID_MOUSE, data,
					 sizeof(data), NULL, NULL);
	if (err != 0) {
		shell_error(sh, "Failed to send mouse report (err %d)", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_send(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t data[CONFIG_BT_HIDS_MAX_REPORT_LEN];
	unsigned long report_id;
	size_t len;
	int err = 0;

	if (!hogp_registered) {
		shell_error(sh, "HOGP Device not registered");
		return -ENOEXEC;
	}

	report_id = shell_strtoul(argv[1], 0, &err);
	if (err != 0 || report_id > UINT8_MAX) {
		shell_error(sh, "Invalid report ID '%s'", argv[1]);
		return -EINVAL;
	}

	len = hex2bin(argv[2], strlen(argv[2]), data, sizeof(data));
	if (len == 0) {
		shell_error(sh, "Invalid data '%s'", argv[2]);
		return -EINVAL;
	}

	err = bt_hogp_device_send_report(default_conn, (uint8_t)report_id, data, (uint16_t)len,
					 NULL, NULL);
	if (err != 0) {
		shell_error(sh, "Failed to send report (err %d)", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_value(const struct shell *sh, size_t argc, char *argv[])
{
	struct hogp_shell_value *value;
	unsigned long report_id;
	size_t len;
	int err = 0;

	report_id = shell_strtoul(argv[1], 0, &err);
	if (err != 0 || report_id > UINT8_MAX) {
		shell_error(sh, "Invalid report ID '%s'", argv[1]);
		return -EINVAL;
	}

	value = value_lookup((uint8_t)report_id);
	if (value == NULL) {
		shell_error(sh, "Unknown report ID %lu", report_id);
		return -EINVAL;
	}

	len = hex2bin(argv[2], strlen(argv[2]), value->data, sizeof(value->data));
	if (len == 0) {
		shell_error(sh, "Invalid data '%s'", argv[2]);
		return -EINVAL;
	}

	value->len = (uint8_t)len;

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	hogp_cmds,
	SHELL_CMD_ARG(register, NULL, HELP_NONE, cmd_register, 1, 0),
	SHELL_CMD_ARG(unregister, NULL, HELP_NONE, cmd_unregister, 1, 0),
	SHELL_CMD_ARG(info, NULL, HELP_NONE, cmd_info, 1, 0),
	SHELL_CMD_ARG(mouse, NULL, "<buttons hex> <dx> <dy>", cmd_mouse, 4, 0),
	SHELL_CMD_ARG(send, NULL, "<report id> <data hex>", cmd_send, 3, 0),
	SHELL_CMD_ARG(value, NULL, "<report id> <data hex>", cmd_value, 3, 0),
	SHELL_SUBCMD_SET_END);

static int cmd_hogp(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);

		/* shell returns 1 when help is printed */
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(hogp, &hogp_cmds, "Bluetooth HOGP Device shell commands", cmd_hogp, 1, 1);
