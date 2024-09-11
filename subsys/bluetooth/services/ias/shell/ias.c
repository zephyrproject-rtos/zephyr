/**
 * @file
 * @brief Shell APIs for Bluetooth IAS
 *
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>

#include <zephyr/types.h>
#include <zephyr/shell/shell.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/services/ias.h>

#include "host/shell/bt.h"

extern const struct shell *ctx_shell;

static void alert_stop(void)
{
	shell_print(ctx_shell, "Alert stopped\n");
}

static void alert_start(void)
{
	shell_print(ctx_shell, "Mild alert started\n");
}

static void alert_high_start(void)
{
	shell_print(ctx_shell, "High alert started\n");
}

BT_IAS_CB_DEFINE(ias_callbacks) = {
	.no_alert = alert_stop,
	.mild_alert = alert_start,
	.high_alert = alert_high_start,
};

static int cmd_ias_local_alert_stop(const struct shell *sh, size_t argc, char **argv)
{
	const int result = bt_ias_local_alert_stop();

	if (result) {
		shell_print(sh, "Local alert stop failed: %d", result);
	} else {
		shell_print(sh, "Local alert stopped");
	}

	return result;
}

static int cmd_ias(const struct shell *sh, size_t argc, char **argv)
{
	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(ias_cmds,
	SHELL_CMD_ARG(local_alert_stop, NULL,
		      "Stop alert locally",
		      cmd_ias_local_alert_stop, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(ias, &ias_cmds, "Bluetooth IAS shell commands",
		       cmd_ias, 1, 1);
