/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/settings/settings.h>

#define DT_DRV_COMPAT nxp_bt_hci_uart

#if defined(CONFIG_HCI_NXP_CONFIG_IR)
extern int bt_nxp_trigger_ir(const struct device *dev);
#endif

static int cmd_default_handler(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);
	return -EINVAL;
}

#if defined(CONFIG_HCI_NXP_CONFIG_IR)
static int cmd_trigger_ir(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	static const struct device *uart_dev = DEVICE_DT_GET(DT_INST_GPARENT(0));

	if (!device_is_ready(uart_dev)) {
		err = -ENODEV;
	} else {
		shell_print(sh, "Triggering IR");
		err = bt_nxp_trigger_ir(uart_dev);
		if (err) {
			shell_error(sh, "IR Trigger failed (err %d)", err);
		} else {
			shell_print(sh, "IR recovery done, Load BT settings");
			err = settings_load();
			if (err) {
				shell_error(sh, "settings load failed after IR (err %d)", err);
			} else {
				shell_print(sh, "BT Init success after IR operation");
			}
		}
	}

	return err;
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(bt_nxp_set_cmds,
#if defined(CONFIG_HCI_NXP_CONFIG_IR)
	SHELL_CMD_ARG(trigger_ir, NULL,
		      "Trigger NXP IR configuration",
		      cmd_trigger_ir, 1, 0),
#endif
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(bt_nxp, &bt_nxp_set_cmds, "BT NXP Shell Commands", cmd_default_handler);
