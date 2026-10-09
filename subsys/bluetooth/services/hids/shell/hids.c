/** @file
 *  @brief Bluetooth HID Service shell commands.
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
#define HIDS_SHELL_REPORT_ID_MOUSE   0x01U
#define HIDS_SHELL_REPORT_ID_LED     0x02U
#define HIDS_SHELL_REPORT_ID_FEATURE 0x03U

#define HIDS_SHELL_MOUSE_REPORT_LEN 3
#define HIDS_SHELL_MAX_VALUE_LEN    8

/* Mouse (Report ID 1, Input) + Keyboard LEDs (Report ID 2, Output) +
 * vendor defined (Report ID 3, Feature).
 */
/* The indentation mirrors the HID collection nesting. */
/* clang-format off */
static const uint8_t hids_shell_report_map[] = {
	HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
	HID_USAGE(HID_USAGE_GEN_DESKTOP_MOUSE),
	HID_COLLECTION(HID_COLLECTION_APPLICATION),
		HID_REPORT_ID(HIDS_SHELL_REPORT_ID_MOUSE),
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
		HID_REPORT_ID(HIDS_SHELL_REPORT_ID_LED),
		HID_USAGE_PAGE(HID_USAGE_GEN_LEDS),
		HID_USAGE_MIN8(1),
		HID_USAGE_MAX8(5),
		HID_REPORT_SIZE(1),
		HID_REPORT_COUNT(5),
		HID_OUTPUT(0x02),
		HID_REPORT_SIZE(3),
		HID_REPORT_COUNT(1),
		HID_OUTPUT(0x03),
		HID_REPORT_ID(HIDS_SHELL_REPORT_ID_FEATURE),
		HID_USAGE_PAGE16(0xFF00),
		HID_USAGE(0x01),
		HID_LOGICAL_MIN8(0),
		HID_LOGICAL_MAX16(0xFF, 0x00),
		HID_REPORT_SIZE(8),
		HID_REPORT_COUNT(1),
		HID_FEATURE(0x02),
	HID_END_COLLECTION,
};
/* clang-format on */

/* The Reports the commands register. The HID Service holds their values, so
 * these are only the initial ones and the Report Type each Report ID belongs to.
 */
static const struct hids_shell_report {
	uint8_t id;
	uint8_t type;
	uint8_t len;
	uint8_t init[HIDS_SHELL_MAX_VALUE_LEN];
} hids_shell_reports[] = {
	{.id = HIDS_SHELL_REPORT_ID_MOUSE,
	 .type = BT_HID_REPORT_TYPE_INPUT,
	 .len = HIDS_SHELL_MOUSE_REPORT_LEN},
	{.id = HIDS_SHELL_REPORT_ID_LED, .type = BT_HID_REPORT_TYPE_OUTPUT, .len = 1},
	{.id = HIDS_SHELL_REPORT_ID_FEATURE, .type = BT_HID_REPORT_TYPE_FEATURE, .len = 1},
};

static bool hids_registered;

static const struct hids_shell_report *report_lookup(uint8_t report_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(hids_shell_reports); i++) {
		if (hids_shell_reports[i].id == report_id) {
			return &hids_shell_reports[i];
		}
	}

	return NULL;
}

static void hids_set_report(struct bt_conn *conn, uint8_t report_type, uint8_t report_id,
			    const uint8_t *data, uint16_t len)
{
	ARG_UNUSED(conn);

	/* The service stores the value, so this only reports the write */
	bt_shell_print("HIDS: SET_REPORT type %u id %u: %s", report_type, report_id,
		       bt_hex(data, len));
}

static void hids_protocol_mode_changed(struct bt_conn *conn, uint8_t protocol)
{
	ARG_UNUSED(conn);

	bt_shell_print("HIDS: Protocol Mode changed to %s",
		       protocol == BT_HID_PROTOCOL_BOOT ? "Boot" : "Report");
}

/* No default case, so that a new HID Control Point command does not silently
 * get the name of an existing one.
 */
static const char *ctrl_point_str(enum bt_hids_ctrl_point cmd)
{
	switch (cmd) {
	case BT_HIDS_CTRL_SUSPEND:
		return "Suspend";
	case BT_HIDS_CTRL_EXIT_SUSPEND:
		return "Exit Suspend";
	}

	return "unknown";
}

static void hids_ctrl_point(struct bt_conn *conn, enum bt_hids_ctrl_point cmd)
{
	ARG_UNUSED(conn);

	bt_shell_print("HIDS: HID Control Point: %s", ctrl_point_str(cmd));
}

static void hids_ccc_changed(struct bt_conn *conn, uint8_t report_id, uint8_t report_type,
			     bool enabled)
{
	ARG_UNUSED(conn);

	bt_shell_print("HIDS: Notifications %s for report type %u id %u",
		       enabled ? "enabled" : "disabled", report_type, report_id);
}

static const char *boot_report_str(enum bt_hids_boot_report report)
{
	switch (report) {
	case BT_HIDS_BOOT_REPORT_KEYBOARD_INPUT:
		return "Boot Keyboard Input";
	case BT_HIDS_BOOT_REPORT_KEYBOARD_OUTPUT:
		return "Boot Keyboard Output";
	case BT_HIDS_BOOT_REPORT_MOUSE_INPUT:
		return "Boot Mouse Input";
	}

	return "unknown";
}

static void hids_set_boot_report(struct bt_conn *conn, enum bt_hids_boot_report report,
				 const uint8_t *data, uint16_t len)
{
	ARG_UNUSED(conn);

	bt_shell_print("HIDS: SET_REPORT %s: %s", boot_report_str(report), bt_hex(data, len));
}

static void hids_boot_ccc_changed(struct bt_conn *conn, enum bt_hids_boot_report report,
				 bool enabled)
{
	ARG_UNUSED(conn);

	bt_shell_print("HIDS: Notifications %s for the %s Report",
		       enabled ? "enabled" : "disabled", boot_report_str(report));
}

static const struct bt_hids_cb hids_shell_cb = {
	.set_report = hids_set_report,
	.protocol_mode_changed = hids_protocol_mode_changed,
	.ctrl_point = hids_ctrl_point,
	.ccc_changed = hids_ccc_changed,
	.set_boot_report = hids_set_boot_report,
	.boot_ccc_changed = hids_boot_ccc_changed,
};

static int cmd_register(const struct shell *sh, size_t argc, char *argv[])
{
	const struct bt_hids_register_param param = {
		/* clang-format off */
		.info = {
			.bcd_hid = 0x0111,
			.b_country_code = 0x00,
			.flags = BT_HID_INFO_FLAG_REMOTE_WAKE |
				 BT_HID_INFO_FLAG_NORMALLY_CONNECTABLE,
		},
		/* clang-format on */
		.report_map = hids_shell_report_map,
		.report_map_len = sizeof(hids_shell_report_map),
		.input_report_ids = {HIDS_SHELL_REPORT_ID_MOUSE},
		.output_report_ids = {HIDS_SHELL_REPORT_ID_LED},
		.feature_report_ids = {HIDS_SHELL_REPORT_ID_FEATURE},
		.cb = &hids_shell_cb,
	};
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	err = bt_hids_register(&param);
	if (err == 0) {
		/* The service holds the values, so seed them */
		for (size_t i = 0; i < ARRAY_SIZE(hids_shell_reports); i++) {
			const struct hids_shell_report *r = &hids_shell_reports[i];

			(void)bt_hids_report_set(r->type, r->id, r->init, r->len);
		}
	}

	if (err != 0) {
		shell_error(sh, "Failed to register the HID Service (err %d)", err);
		return -ENOEXEC;
	}

	hids_registered = true;
	shell_print(sh, "HID Service registered");

	return 0;
}

static int cmd_unregister(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!hids_registered) {
		shell_error(sh, "HID Service not registered");
		return -ENOEXEC;
	}

	err = bt_hids_unregister();
	if (err != 0) {
		shell_error(sh, "Failed to unregister the HID Service (err %d)", err);
		return -ENOEXEC;
	}

	hids_registered = false;
	shell_print(sh, "HID Service unregistered");

	return 0;
}

static int cmd_info(const struct shell *sh, size_t argc, char *argv[])
{
	enum bt_hid_protocol_mode mode;
	bool suspended;
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Registered:    %s", hids_registered ? "yes" : "no");
	if (!hids_registered) {
		return 0;
	}

	if (default_conn == NULL) {
		shell_print(sh, "Not connected, no per-connection HID state");
	} else {
		err = bt_hids_get_protocol_mode(default_conn, &mode);
		if (err != 0) {
			shell_error(sh, "Failed to get Protocol Mode (err %d)", err);
			return -ENOEXEC;
		}

		err = bt_hids_get_suspend_state(default_conn, &suspended);
		if (err != 0) {
			shell_error(sh, "Failed to get Suspend state (err %d)", err);
			return -ENOEXEC;
		}

		shell_print(sh, "Protocol Mode: %s",
			    mode == BT_HID_PROTOCOL_BOOT ? "Boot" : "Report");
		shell_print(sh, "Suspended:     %s", suspended ? "yes" : "no");
	}

	for (size_t i = 0; i < ARRAY_SIZE(hids_shell_reports); i++) {
		const struct hids_shell_report *report = &hids_shell_reports[i];
		uint8_t value[HIDS_SHELL_MAX_VALUE_LEN];
		uint16_t value_len = sizeof(value);

		err = bt_hids_report_get(report->type, report->id, value, &value_len);
		if (err != 0) {
			shell_error(sh, "Failed to read Report %u: %d", report->id, err);
			return -ENOEXEC;
		}

		shell_print(sh, "Report %u:      %s", report->id, bt_hex(value, value_len));
	}

	return 0;
}

static int cmd_mouse(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t data[HIDS_SHELL_MOUSE_REPORT_LEN];
	unsigned long buttons;
	long delta;
	int err = 0;

	if (!hids_registered) {
		shell_error(sh, "HID Service not registered");
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

	err = bt_hids_send_report(default_conn, HIDS_SHELL_REPORT_ID_MOUSE, data, sizeof(data),
				  NULL, NULL);
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

	if (!hids_registered) {
		shell_error(sh, "HID Service not registered");
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

	err = bt_hids_send_report(default_conn, (uint8_t)report_id, data, (uint16_t)len, NULL,
				  NULL);
	if (err != 0) {
		shell_error(sh, "Failed to send report (err %d)", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_value(const struct shell *sh, size_t argc, char *argv[])
{
	const struct hids_shell_report *report;
	uint8_t tmp[HIDS_SHELL_MAX_VALUE_LEN];
	unsigned long report_id;
	size_t len;
	int err = 0;

	report_id = shell_strtoul(argv[1], 0, &err);
	if (err != 0 || report_id > UINT8_MAX) {
		shell_error(sh, "Invalid report ID '%s'", argv[1]);
		return -EINVAL;
	}

	report = report_lookup((uint8_t)report_id);
	if (report == NULL) {
		shell_error(sh, "Unknown report ID %lu", report_id);
		return -EINVAL;
	}

	/* Parse into a temporary buffer so that a malformed argument leaves the
	 * value alone.
	 */
	len = hex2bin(argv[2], strlen(argv[2]), tmp, sizeof(tmp));
	if (len == 0) {
		shell_error(sh, "Invalid data '%s'", argv[2]);
		return -EINVAL;
	}

	err = bt_hids_report_set(report->type, report->id, tmp, (uint16_t)len);
	if (err != 0) {
		shell_error(sh, "Failed to set the Report value: %d", err);
		return -ENOEXEC;
	}

	return 0;
}

static int boot_report_parse(const struct shell *sh, const char *name,
			     enum bt_hids_boot_report *report)
{
	if (strcmp(name, "kb-in") == 0) {
		*report = BT_HIDS_BOOT_REPORT_KEYBOARD_INPUT;
	} else if (strcmp(name, "kb-out") == 0) {
		*report = BT_HIDS_BOOT_REPORT_KEYBOARD_OUTPUT;
	} else if (strcmp(name, "mouse-in") == 0) {
		*report = BT_HIDS_BOOT_REPORT_MOUSE_INPUT;
	} else {
		shell_error(sh, "Unknown Boot Report '%s'", name);
		return -EINVAL;
	}

	return 0;
}

static int cmd_boot_send(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t data[BT_HIDS_BOOT_KB_IN_LEN];
	enum bt_hids_boot_report report;
	size_t len;
	int err;

	ARG_UNUSED(argc);

	err = boot_report_parse(sh, argv[1], &report);
	if (err != 0) {
		return err;
	}

	len = hex2bin(argv[2], strlen(argv[2]), data, sizeof(data));
	if (len == 0) {
		shell_error(sh, "Invalid data '%s'", argv[2]);
		return -EINVAL;
	}

	err = bt_hids_boot_report_send(default_conn, report, data, (uint16_t)len, NULL, NULL);
	if (err != 0) {
		shell_error(sh, "Failed to send the %s Report: %d", boot_report_str(report), err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_boot_value(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t data[BT_HIDS_BOOT_KB_IN_LEN];
	enum bt_hids_boot_report report;
	size_t len;
	int err;

	ARG_UNUSED(argc);

	err = boot_report_parse(sh, argv[1], &report);
	if (err != 0) {
		return err;
	}

	len = hex2bin(argv[2], strlen(argv[2]), data, sizeof(data));
	if (len == 0) {
		shell_error(sh, "Invalid data '%s'", argv[2]);
		return -EINVAL;
	}

	err = bt_hids_boot_report_set(report, data, (uint16_t)len);
	if (err != 0) {
		shell_error(sh, "Failed to set the %s Report value: %d", boot_report_str(report),
			    err);
		return -ENOEXEC;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	hids_cmds, SHELL_CMD_ARG(register, NULL, HELP_NONE, cmd_register, 1, 0),
	SHELL_CMD_ARG(unregister, NULL, HELP_NONE, cmd_unregister, 1, 0),
	SHELL_CMD_ARG(info, NULL, HELP_NONE, cmd_info, 1, 0),
	SHELL_CMD_ARG(mouse, NULL, "<buttons hex> <dx> <dy>", cmd_mouse, 4, 0),
	SHELL_CMD_ARG(send, NULL, "<report id> <data hex>", cmd_send, 3, 0),
	SHELL_CMD_ARG(value, NULL, "<report id> <data hex>", cmd_value, 3, 0),
	SHELL_CMD_ARG(boot-send, NULL, "<kb-in|mouse-in> <data hex>", cmd_boot_send, 3, 0),
	SHELL_CMD_ARG(boot-value, NULL, "<kb-in|kb-out|mouse-in> <data hex>", cmd_boot_value, 3, 0),
	SHELL_SUBCMD_SET_END);

static int cmd_hids(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);

		/* shell returns 1 when help is printed */
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(hids, &hids_cmds, "Bluetooth HID Service shell commands", cmd_hids, 1, 1);
