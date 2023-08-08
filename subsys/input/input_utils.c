/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/input/input.h>
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

static void input_cb(struct input_event *evt)
{
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
INPUT_CALLBACK_DEFINE(NULL, input_cb);
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

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_input_cmds,
#ifdef CONFIG_INPUT_EVENT_DUMP
	SHELL_CMD_ARG(dump, NULL,
		      "Enable event dumping\n"
		      "usage: dump <on|off>",
		      input_cmd_dump, 2, 0),
#endif /* CONFIG_INPUT_EVENT_DUMP */
	SHELL_CMD_ARG(report, NULL,
		      "Trigger an input report event\n"
		      "usage: report <type> <code> <value> [<sync>]",
		      input_cmd_report, 4, 1),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(input, &sub_input_cmds, "Input commands", NULL);
#endif /* CONFIG_INPUT_SHELL */
