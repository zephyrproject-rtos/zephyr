/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <zephyr/bluetooth/bluetooth.h>

/* TODO: Move to proper header file */
#if defined(CONFIG_HCI_NXP_CONFIG_IR)
extern void bt_nxp_trigger_ir(void);
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
	bt_nxp_trigger_ir();
	shell_print(sh, "IR configuration triggered");
	return 0;
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
