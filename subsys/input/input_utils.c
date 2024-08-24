/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/input/input.h>
#ifdef CONFIG_INPUT_SHELL_KBD_MATRIX_STATE
#include <zephyr/input/input_kbd_matrix.h>
#endif
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/atomic.h>

LOG_MODULE_DECLARE(input);

#ifdef CONFIG_INPUT_EVENT_DUMP
#ifdef CONFIG_INPUT_SHELL
static atomic_t dump_enable;

static bool input_dump_enabled(void)
{
	return atomic_get(&dump_enable);
}

static int input_cmd_dump(const struct shell *sh, size_t argc, char *argv[])
{
	bool enabled;
	int err = 0;

	enabled = shell_strtobool(argv[1], 0, &err);
	if (err) {
		shell_error(sh, "Invalid argument: %s", argv[1]);
		return err;
	}

	if (enabled) {
		shell_info(sh, "Input event dumping enabled");
		atomic_set(&dump_enable, 1);
	} else {
		atomic_set(&dump_enable, 0);
	}

	return 0;
}
#else
static bool input_dump_enabled(void)
{
	return true;
}
#endif /* CONFIG_INPUT_SHELL */

static void input_dump_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	if (!input_dump_enabled()) {
		return;
	}

	LOG_INF("input event: dev=%-16s %3s type=%2x code=%3d value=%d",
		evt->dev ? evt->dev->name : "NULL",
		evt->sync ? "SYN" : "",
		evt->type,
		evt->code,
		evt->value);
}
INPUT_CALLBACK_DEFINE(NULL, input_dump_cb, NULL);
#endif /* CONFIG_INPUT_EVENT_DUMP */

#ifdef CONFIG_INPUT_SHELL
static int input_cmd_report(const struct shell *sh, size_t argc, char *argv[])
{
	bool sync;
	int err = 0;
	uint32_t type, code, value;

	if (argc == 5) {
		sync = shell_strtobool(argv[4], 0, &err);
		if (err) {
			shell_error(sh, "Invalid argument: %s", argv[4]);
			return err;
		}
	} else {
		sync = true;
	}

	type = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_error(sh, "Invalid argument: %s", argv[1]);
		return err;
	}
	if (type > UINT8_MAX) {
		shell_error(sh, "Out of range: %s", argv[1]);
		return -EINVAL;
	}

	code = shell_strtoul(argv[2], 0, &err);
	if (err) {
		shell_error(sh, "Invalid argument: %s", argv[2]);
		return err;
	}
	if (code > UINT16_MAX) {
		shell_error(sh, "Out of range: %s", argv[2]);
		return -EINVAL;
	}

	value = shell_strtoul(argv[3], 0, &err);
	if (err) {
		shell_error(sh, "Invalid argument: %s", argv[3]);
		return err;
	}

	input_report(NULL, type, code, value, sync, K_FOREVER);

	return 0;
}

#ifdef CONFIG_INPUT_SHELL_KBD_MATRIX_STATE
static const struct device *kbd_matrix_state_dev;
static kbd_row_t kbd_matrix_state[CONFIG_INPUT_SHELL_KBD_MATRIX_STATE_MAX_COLS];
static kbd_row_t kbd_matrix_key_mask[CONFIG_INPUT_SHELL_KBD_MATRIX_STATE_MAX_COLS];

/* Keep space for each column value, 2 char per byte + space. */
#define KEY_MATRIX_ENTRY_LEN (sizeof(kbd_row_t) * 2 + 1)
#define KEY_MATRIX_BUF_SZ (CONFIG_INPUT_SHELL_KBD_MATRIX_STATE_MAX_COLS * \
			   KEY_MATRIX_ENTRY_LEN)
static char kbd_matrix_buf[KEY_MATRIX_BUF_SZ];

static void kbd_matrix_state_log_entry(char *header, kbd_row_t *data)
{
	const struct input_kbd_matrix_common_config *cfg = kbd_matrix_state_dev->config;
	char *buf = kbd_matrix_buf;
	int size = sizeof(kbd_matrix_buf);
	int ret;
	char blank[KEY_MATRIX_ENTRY_LEN];
	int count = 0;

	memset(blank, '-', sizeof(blank) - 1);
	blank[sizeof(blank) - 1] = '\0';

	for (int i = 0; i < cfg->col_size; i++) {
		char *sep = (i + 1) < cfg->col_size ? " " : "";

		if (data[i] != 0) {
			ret = snprintf(buf, size, "%" PRIkbdrow "%s", data[i], sep);
		} else {
			ret = snprintf(buf, size, "%s%s", blank, sep);
		}
		size -= ret;
		buf += ret;

		count += POPCOUNT(data[i]);

		/* Last byte is for the string termination */
		if (size < 1) {
			LOG_ERR("kbd_matrix_buf too small");
			return;
		}
	}

	LOG_INF("%s %s [%s] (%d)",
		kbd_matrix_state_dev->name, header, kbd_matrix_buf, count);
}

static void kbd_matrix_state_log(struct input_event *evt, void *user_data)
{
	const struct input_kbd_matrix_common_config *cfg;
	static uint32_t row, col;
	static bool val;

	ARG_UNUSED(user_data);

	if (kbd_matrix_state_dev == NULL || kbd_matrix_state_dev != evt->dev) {
		return;
	}

	cfg = kbd_matrix_state_dev->config;

	switch (evt->code) {
	case INPUT_ABS_X:
		col = evt->value;
		break;
	case INPUT_ABS_Y:
		row = evt->value;
		break;
	case INPUT_BTN_TOUCH:
		val = evt->value;
		break;
	}

	if (!evt->sync) {
		return;
	}

	if (col > (CONFIG_INPUT_SHELL_KBD_MATRIX_STATE_MAX_COLS - 1)) {
		LOG_ERR("column index too large for the state buffer: %d", col);
		return;
	}

	if (col > (cfg->col_size - 1)) {
		LOG_ERR("invalid column index: %d", col);
		return;
	}

	if (row > (cfg->row_size - 1)) {
		LOG_ERR("invalid row index: %d", row);
		return;
	}

	WRITE_BIT(kbd_matrix_state[col], row, val);
	if (val != 0) {
		WRITE_BIT(kbd_matrix_key_mask[col], row, 1);
	}

	kbd_matrix_state_log_entry("state", kbd_matrix_state);
}
INPUT_CALLBACK_DEFINE(NULL, kbd_matrix_state_log, NULL);

static int input_cmd_kbd_matrix_state_dump(const struct shell *sh,
					   size_t argc, char *argv[])
{
	const struct device *dev;

	if (!strcmp(argv[1], "off")) {
		if (kbd_matrix_state_dev != NULL) {
			kbd_matrix_state_log_entry("key-mask",
						   kbd_matrix_key_mask);
		}

		kbd_matrix_state_dev = NULL;
		shell_info(sh, "Keyboard state logging disabled");
		return 0;
	}

	dev = device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "Invalid device: %s", argv[1]);
		return -ENODEV;
	}

	if (kbd_matrix_state_dev != NULL && kbd_matrix_state_dev != dev) {
		shell_error(sh, "Already logging for %s, disable logging first",
			    kbd_matrix_state_dev->name);
		return -EINVAL;
	}

	memset(kbd_matrix_state, 0, sizeof(kbd_matrix_state));
	memset(kbd_matrix_key_mask, 0, sizeof(kbd_matrix_state));
	kbd_matrix_state_dev = dev;

	shell_info(sh, "Keyboard state logging enabled for %s", dev->name);

	return 0;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);
#endif /* CONFIG_INPUT_SHELL_KBD_MATRIX_STATE */

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_input_cmds,
#ifdef CONFIG_INPUT_EVENT_DUMP
	SHELL_CMD_ARG(dump, NULL,
		      "Enable event dumping\n"
		      "usage: dump <on|off>",
		      input_cmd_dump, 2, 0),
#endif /* CONFIG_INPUT_EVENT_DUMP */
#ifdef CONFIG_INPUT_SHELL_KBD_MATRIX_STATE
	SHELL_CMD_ARG(kbd_matrix_state_dump, &dsub_device_name,
		      "Print the state of a keyboard matrix device each time a "
		      "key is pressed or released\n"
		      "usage: kbd_matrix_state_dump <device>|off",
		      input_cmd_kbd_matrix_state_dump, 2, 0),
#endif /* CONFIG_INPUT_SHELL_KBD_MATRIX_STATE */
	SHELL_CMD_ARG(report, NULL,
		      "Trigger an input report event\n"
		      "usage: report <type> <code> <value> [<sync>]",
		      input_cmd_report, 4, 1),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(input, &sub_input_cmds, "Input commands", NULL);
#endif /* CONFIG_INPUT_SHELL */
